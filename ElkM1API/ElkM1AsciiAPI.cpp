/*
	ElkM1AsciiAPI.cpp: Provides an implementation of the M1AsciiAPI class, using a high-performance function
	  table and blocks using mutexes with condition variables.
	@author Zach Jaggi
*/
#include "ElkM1AsciiAPI.h"
#include <iostream>
#include <stdio.h>
#include <limits.h>

namespace Elk {
#pragma region Static Helper Functions
	std::vector<char> M1AsciiAPI::toAsciiHex(int value, int length) {
		// TODO: length, value not negative

		std::vector<char> buff = std::vector<char>(length >= 5 ? length + 1 : 5);
		buff.resize(sprintf(&buff[0], "%0*.*X", length, length, value));
		if (buff.size() > length)
			buff.erase(buff.begin(), buff.begin() + (buff.size() - length));
		return buff;
	}
	std::vector<char> M1AsciiAPI::toAsciiDec(int value, int length) {
		// TODO: length, value not negative

		std::vector<char> buff = std::vector<char>(length >= 7 ? length + 1 : 7);
		buff.resize(sprintf(&buff[0], "%0*.*d", length, length, value));
		if (buff.size() > length)
			buff.erase(buff.begin(), buff.begin() + (buff.size() - length));
		return buff;
	}
	// Message checksum includes length bytes, but length bytes also account for having a checksum.
	std::vector<char> M1AsciiAPI::genChecksum(std::vector<char> message) {
		int checksum = 0;
		for (char c : message)
			checksum += c;
		checksum = (checksum ^ 0xff) + 1;
		return toAsciiHex(checksum, 2);
	}
	// Used for cache objects which are volatile.
	template <typename T>
	T M1AsciiAPI::cacheRequest(M1Monitor::cacheObject<T>& cacheObj, AsciiMessage request, bool ignoreCache = true, int timeoutMillis = 0) {
		// Check if the cache is new enough and return that if it is
		if (!ignoreCache && (cacheObj.age() <= 3))
			return cacheObj.get();

		// If not, send our request and await the response.
		connection->Send(request.getTransmittable());

		return cacheObj.awaitNew(timeoutMillis);
	}
	// Used for cache objects which only change once per M1 programming.
	template <typename T>
	T M1AsciiAPI::cacheExistsRequest(M1Monitor::cacheObject<T>& cacheObj, AsciiMessage request) {
		// Check if the cache has been initialized
		if (cacheObj.isInitialized())
			return cacheObj.get();

		// If not, send our request and await the response.
		connection->Send(request.getTransmittable());

		return cacheObj.awaitNew(1500);
	}
#pragma endregion Static Helper Functions

#pragma region Protocol implementations

	// TODO: Split all std::cout calls into a debug message callback.

	// TODO: Optimization: Any operation which has both "get all" and "get one" operations, 
	//   detect something iterating over it (rapid sequential requests) and fill cache with 'all'
	//   version of command 
	M1AsciiAPI::M1AsciiAPI(M1Connection* conn) : M1Monitor(conn) {
		// Fill up our function table here
		fillFunctionTable();
	}

	// All incoming message handlers defined here. This uses an O(1) function table to handle new data, that is to say
	//   no matter how many commands are added, it will always take the same amount of time to find a command and handle
	//   the data. Message does not include length/checksum/crlf.
	void M1AsciiAPI::fillFunctionTable() {
		// Ignored:
		// XK

		// TODO: Any command which has optional, unrequested reporting, we can leave a callback in the main API
		//   that lets the program do whatever with it.
		// PC

		// Counter value read
		handleMessageTable.emplace("CV", [this](std::string message) {
			// CVNNDDDDD00
			int counter = std::stoi(message.substr(2, 2)) - 1;
			int value = std::stoi(message.substr(4, 5));
			m1cache.counterValues[counter].set((uint16_t)value);
		});
		// Custom value read
		handleMessageTable.emplace("CR", [this](std::string message) {
			// CRNNDDDDD00
			int index = std::stoi(message.substr(2, 2)) - 1;
			if (index == -1) {
				// ALL values returned
				for (int i = 0; i < 20; i++) {
					int value = std::stoi(message.substr(4 + (i * 6), 5));
					m1cache.customValues[i].set((uint16_t)value);
				}
			}
			else {
				// One value returned
				int value = std::stoi(message.substr(4, 5));
				m1cache.customValues[index].set((uint16_t)value);
			}
		});
		// Arming Status Request
		handleMessageTable.emplace("AS", [this](std::string message) {
			std::array<ArmStatus, 8> newStatus;
			for (int i = 0; i < 8; i++) {
				newStatus[i].mode = (ArmMode)(message.at(2 + i) - '0');
				newStatus[i].isReady = (ArmUpMode)(message.at(10 + i) - '0');
				newStatus[i].alarm = (AlarmState)(message.at(18 + i) - '0');
			}
			m1cache.armStatus.set(newStatus);
		});
		// Zone Voltage
		handleMessageTable.emplace("ZV", [this](std::string message) {
			int index = std::stoi(message.substr(2, 3)) - 1;
			int value = std::stoi(message.substr(5, 3));
			m1cache.zoneVoltage[index].set(
				((float)value) / 10
				);
		});
		// Keypad Areas
		handleMessageTable.emplace("KA", [this](std::string message) {
			std::array<int, 16> areas;
			for (int i = 0; i < 16; i++) {
				areas[i] = message.at(2 + i) - '0' - 1;
			}
			m1cache.keypadAreas.set(areas);
		});
		// Zone Partitions
		handleMessageTable.emplace("ZP", [this](std::string message) {
			std::array<int, 208> zones;
			for (int i = 0; i < 208; i++) {
				zones[i] = message.at(2 + i) - '0' - 1;
			}
			m1cache.zonePartitions.set(zones);
		});
		// Control Output Status
		handleMessageTable.emplace("CS", [this](std::string message) {
			std::array<bool, 208> zones;
			for (int i = 0; i < 208; i++) {
				zones[i] = message.at(2 + i) == '1';
			}
			m1cache.controlOutputs.set(zones);
		});
		// Version number, TODO: Scrape XEP number too
		handleMessageTable.emplace("VN", [this](std::string message) {
			std::array<int, 3> vn;
			for (int i = 0; i < 3; i++) {
				vn[i] = stoi(message.substr(2 + (2 * i), 2), 0, 16);
			}
			m1cache.M1VersionNumber.set(vn);
		});
		// Lighting status
		handleMessageTable.emplace("DS", [this](std::string message) {
			int index = stoi(message.substr(2, 3)) - 1;
			int value = stoi(message.substr(5, 2));
			m1cache.lightingStatus[index].set(value);
		});
		// Alarms by zone
		handleMessageTable.emplace("AZ", [this](std::string message) {
			std::array<ZoneDefinition, 208> zones;
			for (int i = 0; i < 208; i++) {
				zones[i] = (ZoneDefinition)(message.at(2 + i) - '0');
			}
			m1cache.zoneAlarms.set(zones);
		});
		// Definitions by zone
		handleMessageTable.emplace("ZD", [this](std::string message) {
			std::array<ZoneDefinition, 208> zones;
			for (int i = 0; i < 208; i++) {
				zones[i] = (ZoneDefinition)(message.at(2 + i) - '0');
			}
			m1cache.zoneDefinitions.set(zones);
		});
		// Zone Statuses
		handleMessageTable.emplace("ZS", [this](std::string message) {
			std::array<ZoneState, 208> zones;
			for (int i = 0; i < 208; i++) {
				int bitfield = message.at(2 + i) - '0';
				zones[i] = { (PhysicalZoneState)(bitfield & 0x3),
					(LogicalZoneState)(bitfield >> 2) };
			}
			m1cache.zoneStatus.set(zones);
		});
		// Zones Bypassed
		handleMessageTable.emplace("ZB", [this](std::string message) {
			int index = std::stoi(message.substr(2, 3)) - 1;
			bool bypassed = message.at(5) == '1';
			if (index == 998 || index == -1)
			{
				// Area bypass
				m1cache.areaBypassed.set(bypassed);
			}
			else {
				m1cache.zonesBypassed[index].set(bypassed);
			}
		});
		// Temperature data block
		handleMessageTable.emplace("LW", [this](std::string message) {
			// Keypad temps
			for (int i = 0; i < 16; i++) {
				int temp = stoi(message.substr(2 + (3 * i), 3)) - 40;
				m1cache.keypadTemperatures[i].set((temp != -40) ? temp : INT_MIN);
			}
			// Zone temps
			for (int i = 0; i < 16; i++) {
				int temp = stoi(message.substr(50 + (3 * i), 3)) - 60;
				m1cache.zoneTemperatures[i].set((temp != -60) ? temp : INT_MIN);
			}
		});
		// Individual temperature data
		handleMessageTable.emplace("ST", [this](std::string message) {
			TemperatureDevice type = (TemperatureDevice)(message.at(2) - '0');
			int index = std::stoi(message.substr(3, 2)) - 1;
			int value = std::stoi(message.substr(5, 3));
			bool defined = (value != 0);
			switch (type) {
			case TEMPDEVICE_ZONE:
				value -= 60;
				m1cache.zoneTemperatures[index].set(defined ? value : INT_MIN);
				break;
			case TEMPDEVICE_KEYPAD:
				value -= 40;
				m1cache.keypadTemperatures[index].set(defined ? value : INT_MIN);
				break;
			case TEMPDEVICE_THERMOSTAT:
				m1cache.thermostatTemperatures[index].set(defined ? value : INT_MIN);
				break;
			}

		});
		// System Trouble Status TODO: Trouble status is more detailed than this
		handleMessageTable.emplace("ST", [this](std::string message) {
			SystemTroubleStatus sts;
			sts.ACFail = message.at(2) == '1';
			sts.boxTamper = message.at(3) == '1';
			sts.communicationError = message.at(4) == '1';
			sts.EEPROMError = message.at(5) == '1';
			sts.lowBattery = message.at(6) == '1';
			sts.overCurrent = message.at(7) == '1';
			sts.telephoneFault = message.at(8) == '1';
			sts.output2 = message.at(9) == '1';
			sts.missingKeypad = message.at(10) == '1';
			sts.zoneExpander = message.at(11) == '1';
			sts.outputExpander = message.at(12) == '1';
			sts.RPRemoteAccess = message.at(13) == '1';
			sts.commonAreaNotArmed = message.at(14) == '1';
			sts.flashMemoryError = message.at(15) == '1';
			sts.securityAlert = message.at(16) == '1';
			sts.serialPortExpander = message.at(17) == '1';
			sts.lostTransmitter = message.at(18) == '1';
			sts.GESmokeCleanMe = message.at(19) == '1';
			sts.ethernet = message.at(20) == '1';
			m1cache.systemTroubleStatus.set(sts);
		});
		// RP connected
		handleMessageTable.emplace("RP", [this](std::string message) {
			// TODO: Invalidate cache

			// TODO: Throw exceptions on all blocked calls (and new calls)

			// If the rpConnectionTrigger callback exists, execute it
			if (rpConnectionTrigger)
				rpConnectionTrigger(true);
		});
		// RP disconnected
		handleMessageTable.emplace("IE", [this](std::string message) {
			if (rpConnectionTrigger)
				rpConnectionTrigger(false);
		});
		// Thermostat data // TODO: Test this!
		handleMessageTable.emplace("TR", [this](std::string message) {
			ThermostatData td;
			int index = std::stoi(message.substr(2, 2)) - 1;
			td.mode = (ThermostatData::ThermostatMode)(message.at(4) - '0');
			td.holdCurrentTemperature = message.at(5) == '1';
			td.fanOn = message.at(6) == '1';
			td.temperature = std::stoi(message.substr(7, 2));
			td.heatSetPoint = std::stoi(message.substr(9, 2));
			td.coolSetPoint = std::stoi(message.substr(11, 2));
			td.humidity = std::stoi(message.substr(13, 2));
			m1cache.thermostatData[index].set(td);
			m1cache.thermostatTemperatures[index].set(td.temperature);
		});
		// Log data
		handleMessageTable.emplace("LD", [this](std::string message) {
			LogEntry le;
			int index = std::stoi(message.substr(18, 3)) - 1;
			if (index == -1) return; // TODO: New log entry callback?
			le.event = std::stoi(message.substr(2, 4));
			le.eventSubjectNumber = std::stoi(message.substr(6, 3));
			le.area = message.at(9) - '1';
			le.hour = std::stoi(message.substr(10, 2));
			le.minute = std::stoi(message.substr(12, 2));
			le.month = std::stoi(message.substr(14, 2));
			le.day = std::stoi(message.substr(16, 2));
			le.dayOfWeek = (Weekday)(message.at(21) - '0');
			le.year = std::stoi(message.substr(22, 2));
			m1cache.logData[index].set(le);
		});
		// Strings
		handleMessageTable.emplace("SD", [this](std::string message) {
			TextDescriptionType tdt = TextDescriptionType(stoi(message.substr(2, 2)));
			int index = stoi(message.substr(4, 3)) - 1;
			std::string desc = message.substr(7, 16);
			// Normally std::isspace would be good practice, but we're using ASCII so there's only the one.
			desc.erase(std::find_if(desc.rbegin(), desc.rend(), [](char c) { return c != ' '; }).base(), desc.end());
			switch (tdt) {
			case TDT_ZoneName:
				return m1cache.ZoneNames[index].set(desc);
			case TDT_AreaName:
				return m1cache.AreaNames[index].set(desc);
			case TDT_UserName:
				return m1cache.UserNames[index].set(desc);
			case TDT_KeypadName:
				return m1cache.KeypadNames[index].set(desc);
			case TDT_OutputName:
				return m1cache.OutputNames[index].set(desc);
			case TDT_TaskName:
				return m1cache.TaskNames[index].set(desc);
			case TDT_TelephoneName:
				return m1cache.TelephoneNames[index].set(desc);
			case TDT_LightName:
				return m1cache.LightNames[index].set(desc);
			case TDT_AlarmDurationName:
				return m1cache.AlarmDurationNames[index].set(desc);
			case TDT_CustomSettings:
				return m1cache.CustomSettingNames[index].set(desc);
			case TDT_CounterName:
				return m1cache.CounterNames[index].set(desc);
			case TDT_ThermostatName:
				return m1cache.ThermostatNames[index].set(desc);
			case TDT_FKEY1:
				return m1cache.FKEY1s[index].set(desc);
			case TDT_FKEY2:
				return m1cache.FKEY2s[index].set(desc);
			case TDT_FKEY3:
				return m1cache.FKEY3s[index].set(desc);
			case TDT_FKEY4:
				return m1cache.FKEY4s[index].set(desc);
			case TDT_FKEY5:
				return m1cache.FKEY5s[index].set(desc);
			case TDT_FKEY6:
				return m1cache.FKEY6s[index].set(desc);
			case TDT_AudioZoneName:
				return m1cache.AudioZoneNames[index].set(desc);
			case TDT_AudioSourceName:
				return m1cache.AudioSourceNames[index].set(desc);
			}
			std::cout << "Error parsing message: " << message << "\n";
		});
		// Keypad function press TODO: Test
		handleMessageTable.emplace("KF", [this](std::string message) {
			std::array<ChimeMode, 8> chimeModes;
			for (int i = 0; i < 8; i++) {
				chimeModes[i] = (ChimeMode)(message.at(5 + i) - '0');
			}
			m1cache.chimeModes.set(chimeModes);
		});
		// Returned PLC status
		handleMessageTable.emplace("PS", [this](std::string message) {
			std::array<int, 64> lightingLevels;
			int bank = message.at(2) - '0';
			for (int i = 0; i < 64; i++) {
				lightingLevels[i] = message.at(3 + i) - '0';
			}
			m1cache.plcStatus[bank].set(lightingLevels);
		});
		// AudioZone audio data
		handleMessageTable.emplace("CA", [this](std::string message) {
			AudioData data;
			int index = stoi(message.substr(2, 2)) - 1;
			data.zoneIsOn = message.at(4) == '1';
			data.source = stoi(message.substr(5, 2)) - 1;
			data.volume = stoi(message.substr(7, 3));
			data.bass = stoi(message.substr(10, 3));
			data.treble = stoi(message.substr(13, 3));
			data.loudness = message.at(16) == '1';
			data.balance = stoi(message.substr(17, 3));
			data.partyMode = (AudioData::PartyMode)(message.at(20) - '0');
			data.doNotDisturb = message.at(21) == '1';
			m1cache.audioData[index].set(data);
		});
	}

	std::vector<char> M1AsciiAPI::cutMessage(std::vector<char>& buffer) {
		for (int i = 0; i < buffer.size(); i++) {
			if (buffer.at(i) == '\n')
			{
				// Get message
				std::vector<char> newMessage = std::vector<char>(buffer.begin(), buffer.begin() + i + 1);
				buffer.erase(buffer.begin(), buffer.begin() + i + 1);
				return newMessage;
			}
		}
		return std::vector<char>(); // Return empty vector otherwise.
	}

	void M1AsciiAPI::handleMessage(std::vector<char> message) {
		if (message.size() < 4) {
			message.push_back('\0');
			std::cout << "Caught malformed message: " << std::string(&message[0]) << "\n";
			return;
		}

		// Catch "OK\r\n" TODO: Checked for in every message, perhaps another way?
		if (!memcmp(&message[0], "OK\r\n", 4)) {
			m1cache.okMessage.set(true);
			return;
		}

		// Find identifying bytes (3 and 4)
		std::string identifier(message.begin() + 2, message.begin() + 4);

		try {
			// Call appropriate function
			handleMessageTable.at(identifier)(AsciiMessage::fromTransmission(message).to_string());
		}
		catch (...) {
			message.push_back('\0');
			std::cout << "No handler for message: " << std::string(&message[0]) << "\n";
		}
	}

#pragma endregion Protocol implementations

#pragma region M1API Implementations
	// TODO: Make forEach implementations for all device types.
	void M1AsciiAPI::forEachConfiguredZone(std::function<void(int)> funct) {
		std::array<ZoneDefinition, 208> zdef = getZoneDefinitions();
		for (int i = 0; i < 208; i++) {
			if (zdef[i] != ZONEDEF_DISABLED) {
				funct(i);
			}
		}
	}
	// TODO: Function to intelligently collect names, with 150ms timeout on missed names that skips to next section
	void M1AsciiAPI::collectAllNames() {
		throw std::exception("Not imlemented.");
	}
	// TODO: Replace magic numbers in timeout definitions with defines
	// TODO: Go back over functions, ensure 0-index for appropriate data
	// TODO: Add more optional-timeout methods
	// TODO: Check each command for version requirement.
	
	M1AsciiAPI::AudioData M1AsciiAPI::getAudioData(int audioZone) { 
		// TODO: Can't test without something to test against.
		AsciiMessage message("ca");
		message += toAsciiDec(audioZone + 1, 2);
		message += "00";
		return cacheRequest(m1cache.audioData[audioZone], message, false, 1500);
	}
	bool M1AsciiAPI::setAreaBypass(int area, std::string pinCode, bool bypassed) {
		// TODO: Test this more!
		// TODO: 0 <= zone <= 207
		if (pinCode.size() < 6)
			pinCode.insert(0, 6 - pinCode.size(), '0');
		AsciiMessage message("zb");
		message += bypassed ? "999" : "000";
		message += toAsciiDec(area + 1, 1);
		message += pinCode;
		message += "00";
		return cacheRequest(m1cache.areaBypassed, message, true, 0);
	}
	bool M1AsciiAPI::zoneBypass(int zone, std::string pinCode) {
		// TODO: 0 <= zone <= 207
		if (pinCode.size() < 6)
			pinCode.insert(0, 6 - pinCode.size(), '0');
		AsciiMessage message("zb");
		message += toAsciiDec(zone + 1, 3);
		message += "0"; // otherwise area
		message += pinCode;
		message += "00";
		return cacheRequest(m1cache.zonesBypassed[zone], message, true, 0);
	}
	float M1AsciiAPI::getZoneVoltage(int zone) {
		return getZoneVoltage(zone, false, 0);
	}
	float M1AsciiAPI::getZoneVoltage(int zone, bool ignoreCache, int timeoutMillis) {
		//TODO: 0 <= zone <= 207
		AsciiMessage message("zv");
		message += toAsciiDec(zone + 1, 3);
		message += "00";
		return cacheRequest<float>(m1cache.zoneVoltage[zone], message, ignoreCache, timeoutMillis);
	}
	int M1AsciiAPI::getLightingStatus(int device) {
		return getLightingStatus(device, false, 0);
	}
	int M1AsciiAPI::getLightingStatus(int device, bool ignoreCache = false, int timeoutMillis = 0) {
		// TODO: 0 <= device <= 255
		AsciiMessage message("ds");
		message += toAsciiDec(device + 1, 3);
		message += "00";
		return cacheRequest<int>(m1cache.lightingStatus[device], message, ignoreCache, timeoutMillis);
	}
	int M1AsciiAPI::getTemperature(TemperatureDevice type, int device) {
		//TODO: 0 <= device <= 15
		AsciiMessage message("st");
		message += toAsciiDec(type, 1);
		message += toAsciiDec(device + 1, 2);
		message += "00";
		return cacheRequest([device, type, this]()->cacheObject<int>& {
			switch (type){
			case TEMPDEVICE_KEYPAD:
				return m1cache.keypadTemperatures[device];
			case TEMPDEVICE_THERMOSTAT:
				return m1cache.thermostatTemperatures[device];
			case TEMPDEVICE_ZONE:
				return m1cache.zoneTemperatures[device];
			}
		}(), message, false, 0);
	}
	std::array<int, 16> M1AsciiAPI::getTemperatures(TemperatureDevice type) {
		//TODO: 0 <= device <= 15
		std::array<int, 16> reply;
		// There's three different ways to get temperature data. Select the best for the task.
		switch (type)
		{
		case TEMPDEVICE_KEYPAD:
			for (int i = 0; i < 16; i++) {
				reply[i] = cacheRequest(m1cache.keypadTemperatures[i], (AsciiMessage)"lw00", false, 0);
			}
			return reply;
		case TEMPDEVICE_ZONE:
			for (int i = 0; i < 16; i++) {
				reply[i] = cacheRequest(m1cache.zoneTemperatures[i], (AsciiMessage)"lw00", false, 0);
			}
			return reply;
		case TEMPDEVICE_THERMOSTAT:
			for (int i = 0; i < 16; i++) {
				reply[i] = getTemperature(type, i);
			}
			return reply;
		}

	}
	M1AsciiAPI::KeypadFkeyStatus M1AsciiAPI::getKeypadFkeyStatus(int keypad) {
		AsciiMessage message("kc");
		message += toAsciiDec(keypad + 1, 2);
		message += "00";
		return cacheRequest(m1cache.keypadStatuses[keypad], message, true, 0);
	}
	M1AsciiAPI::LogEntry M1AsciiAPI::getLogData(int index) {
		// TODO: 0 <= index <= 509
		// TODO: Log entries move? Never rely on cached values then I guess.
		AsciiMessage request("ld");
		request += toAsciiDec(index + 1, 3);
		request += "00";
		return cacheRequest(m1cache.logData[index], request, true, 0);
	}
	std::array<M1API::LogEntry, 511> M1AsciiAPI::getLogs() {
		// Get log entries. If we find a duplicate, the log has been written to, so insert a new one at the beginning.
		std::vector<LogEntry> logs;
		int duplicates = 0;
		for (int i = 0; i < 511; i++) {
			logs.push_back(getLogData(i));
			if ((i > 0) && memcmp(&logs.at(i), &logs.at(i - 1), sizeof(LogEntry))) {
				// Duplicate log! Make sure to delete it at the end
				duplicates++;
			}
		}
		// Take care of duplicates
		for (int i = 0; i < duplicates; i++) {
			logs.insert(logs.begin() + i, getLogData(i));
			logs.pop_back();
		}
		std::array<LogEntry, 511> le;
		for (int i = 0; i < 511; i++) {
			le[i] = logs[i];
		}
		return le;
	}
	std::array < int, 64> M1AsciiAPI::getPLCStatus(int bank) {
		AsciiMessage request("ps");
		request += toAsciiDec(bank, 1);
		request += "00";
		return cacheRequest(m1cache.plcStatus[bank], request, true, 0);
	}
	M1AsciiAPI::RTCData M1AsciiAPI::getRTCData() { 
		return cacheRequest(m1cache.rtcData, (AsciiMessage)"rr00", true, 0);
	}
	M1AsciiAPI::RTCData M1AsciiAPI::setRTCData(RTCData newData) { 
		AsciiMessage message("rw");
		message += toAsciiDec(newData.seconds, 2);
		message += toAsciiDec(newData.minutes, 2);
		message += toAsciiDec(newData.hours, 2);
		message += toAsciiDec(newData.weekday, 1);
		message += toAsciiDec(newData.day, 2);
		message += toAsciiDec(newData.year, 2);
		message += "00";
		return cacheRequest(m1cache.rtcData, message, true, 0);
	}
	std::array<M1AsciiAPI::ArmStatus, 8> M1AsciiAPI::getArmStatus() {
		return getArmStatus(false, 0);
	}
	std::array<M1AsciiAPI::ArmStatus, 8> M1AsciiAPI::getArmStatus(bool ignoreCache =  false, int timeoutMillis = 0) { 
		return cacheRequest(m1cache.armStatus, (AsciiMessage)"as00", ignoreCache, timeoutMillis);
	}
	std::array<bool, 208> M1AsciiAPI::getControlOutputs() { 
		return cacheRequest(m1cache.controlOutputs, (AsciiMessage)"cs00", true, 0);
	}
	std::array<M1AsciiAPI::ChimeMode, 8> M1AsciiAPI::pressFunctionKey(int keypad, FKEY key) { 
		AsciiMessage message("kf");
		message += toAsciiDec(keypad + 1, 2);
		switch (key) {
		case FKEY_STAR:
			message += "*";
			break;
		case FKEY_CHIME:
			message += "C";
			break;
		default:
			message += toAsciiDec((int)key, 1);
		}
		message += "00";
		return cacheRequest(m1cache.chimeModes, message, true, 0);
	}
	std::array<int, 16> M1AsciiAPI::getKeypadAreas() { 
		return cacheExistsRequest(m1cache.keypadAreas, (AsciiMessage)"ka00");
	}
	std::array<int, 208> M1AsciiAPI::getZonePartitions() {
		return cacheExistsRequest(m1cache.zonePartitions, (AsciiMessage)"zp00");
	}
	std::array<int, 3> M1AsciiAPI::getM1VersionNumber() { 
		return cacheExistsRequest(m1cache.M1VersionNumber, (AsciiMessage)"vn00");
	}
	std::array<uint16_t, 20> M1AsciiAPI::getCustomValues() { 
		std::array<uint16_t, 20> reply;
		for (int i = 0; i < 20; i++) {
			reply[i] = cacheRequest(m1cache.customValues[i], (AsciiMessage)"cp00", false, 0);
		}
		return reply;
	}
	std::array<M1AsciiAPI::ZoneDefinition, 208> M1AsciiAPI::getZoneAlarms() { 
		return cacheRequest(m1cache.zoneAlarms, (AsciiMessage)"az00", true, 0);
	}
	std::array<M1AsciiAPI::ZoneDefinition, 208> M1AsciiAPI::getZoneDefinitions() { 
		return cacheExistsRequest(m1cache.zoneDefinitions, (AsciiMessage)"zd00");
	}
	std::array<M1AsciiAPI::ZoneState, 208> M1AsciiAPI::getZoneStatuses() { 
		return cacheRequest(m1cache.zoneStatus, (AsciiMessage)"zs00", true, 0);
	}
	std::string M1AsciiAPI::getTextDescription(TextDescriptionType type, int index) { 
		// TODO: Bounds
		AsciiMessage message("sd");
		message += toAsciiDec(type, 2);
		message += toAsciiDec(index + 1, 3);
		message += "00";
		switch (type)
		{
		case TDT_ZoneName:
			return cacheExistsRequest(m1cache.ZoneNames[index], message);
		case TDT_AreaName:
			return cacheExistsRequest(m1cache.AreaNames[index], message);
		case TDT_UserName:
			return cacheExistsRequest(m1cache.UserNames[index], message);
		case TDT_KeypadName:
			return cacheExistsRequest(m1cache.KeypadNames[index], message);
		case TDT_OutputName:
			return cacheExistsRequest(m1cache.OutputNames[index], message);
		case TDT_TaskName:
			return cacheExistsRequest(m1cache.TaskNames[index], message);
		case TDT_TelephoneName:
			return cacheExistsRequest(m1cache.TelephoneNames[index], message);
		case TDT_LightName:
			return cacheExistsRequest(m1cache.LightNames[index], message);
		case TDT_AlarmDurationName:
			return cacheExistsRequest(m1cache.AlarmDurationNames[index], message);
		case TDT_CustomSettings:
			return cacheExistsRequest(m1cache.CustomSettingNames[index], message);
		case TDT_CounterName:
			return cacheExistsRequest(m1cache.CounterNames[index], message);
		case TDT_ThermostatName:
			return cacheExistsRequest(m1cache.ThermostatNames[index], message);
		case TDT_FKEY1:
			return cacheExistsRequest(m1cache.FKEY1s[index], message);
		case TDT_FKEY2:
			return cacheExistsRequest(m1cache.FKEY2s[index], message);
		case TDT_FKEY3:
			return cacheExistsRequest(m1cache.FKEY3s[index], message);
		case TDT_FKEY4:
			return cacheExistsRequest(m1cache.FKEY4s[index], message);
		case TDT_FKEY5:
			return cacheExistsRequest(m1cache.FKEY5s[index], message);
		case TDT_FKEY6:
			return cacheExistsRequest(m1cache.FKEY6s[index], message);
		case TDT_AudioZoneName:
			return cacheExistsRequest(m1cache.AudioZoneNames[index], message);
		case TDT_AudioSourceName:
			return cacheExistsRequest(m1cache.AudioSourceNames[index], message);
		}
		throw std::exception("TextDescriptionType not defined.");
	}
	std::vector<char> M1AsciiAPI::getOmnistat2Data(std::vector<char> request) { return std::vector<char>(); }
	M1AsciiAPI::SystemTroubleStatus M1AsciiAPI::getSystemTroubleStatus() {
		return cacheRequest(m1cache.systemTroubleStatus, (AsciiMessage)"ss00", true, 0);
	}
	M1AsciiAPI::ThermostatData M1AsciiAPI::getThermostatData(int index) { 
		// TODO: 0 <= index <= 15
		AsciiMessage request("tr");
		request += toAsciiDec(index + 1, 2);
		request += "00";
		return cacheRequest(m1cache.thermostatData[index], request, false, 0);
	}
	M1AsciiAPI::ThermostatData M1AsciiAPI::setThermostatData(int index, int value, int element) {
		// TODO: bounds, replace element with enum
		AsciiMessage request("ts");
		request += toAsciiDec(index + 1, 2);
		request += toAsciiDec(value, 2);
		request += toAsciiDec(element, 1);
		request += "00";
		return cacheRequest(m1cache.thermostatData[index], request, true, 0);
	}
	uint16_t M1AsciiAPI::getCounterValue(int counter) { 
		return getCounterValue(counter, false, 0); 
	}
	uint16_t M1AsciiAPI::getCounterValue(int counter, bool ignoreCache = false, int timeoutMillis = 0) {
		//TODO: 0 <= counter <= 63
		AsciiMessage message("cv");
		message += toAsciiDec(counter + 1, 2);
		message += "00";
		return cacheRequest(m1cache.counterValues[counter], message, ignoreCache, timeoutMillis);
	}
	uint16_t M1AsciiAPI::getCustomValue(int index) {
		return getCustomValue(index, false, 0);
	}
	uint16_t M1AsciiAPI::getCustomValue(int index, bool ignoreCache = false, int timeoutMillis = 0) {
		//TODO: 0 <= counter <= 19
		AsciiMessage message("cr");
		message += toAsciiDec(index + 1, 2);
		message += "00";
		return cacheRequest(m1cache.customValues[index], message, ignoreCache, timeoutMillis);
	}
	uint16_t M1AsciiAPI::setCounterValue(int counter, uint16_t value) { 
		//TODO: 0 <= counter <= 63
		AsciiMessage message("cx");
		message += toAsciiDec(counter + 1, 2);
		message += toAsciiDec(value, 5);
		message += "00";
		connection->Send(message.getTransmittable());

		// If we have a timeout defined, try to use that, and rethrow exception if we wait too long
		return m1cache.counterValues[counter].awaitNew(0);
	}
	M1AsciiAPI::UserCodeAccess M1AsciiAPI::getUserCodeAccess(std::string userCode) { return UserCodeAccess(); }
	M1AsciiAPI::UserCodeSuccess M1AsciiAPI::requestChangeUserCode(int user, std::string authCode, std::string newUserCode, uint8_t areaMask) { return UserCodeSuccess(); }
	void M1AsciiAPI::activateTask(int taskNumber) {
		//TODO: 0 <= tn <= 31
		AsciiMessage message("tn");
		message += toAsciiDec(taskNumber + 1, 3);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::armDisarm(int area, ArmMode mode, std::string userCode) {
		// TODO: userCode == ascii, 4 or 6 digits
		// TODO: 0 <= area <= 7
		// TODO: 0 <= output < 207
		// userCode = pad left with 0s if 4 digits
		if (userCode.length() < 6)
			userCode.insert(0, 6 - userCode.length(), '0');
		AsciiMessage message("a");

		// ArmLevel TODO: 7, 8 are only in m1ver > 4.2.8
		message += toAsciiDec(mode, 1);
		message += toAsciiDec(area + 1, 1);
		message += userCode;
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::displayLCDText(int area, clearMethod clear, bool beepKeypad, uint16_t displayTime, std::string text) {
		text.resize(32, '^');
		displayLCDText(area, clear, beepKeypad, displayTime, text.substr(0, 16), text.substr(16, 16));
	}
	void M1AsciiAPI::displayLCDText(int area, clearMethod clear, bool beepKeypad, uint16_t displayTime, std::string lineOne, std::string lineTwo) { 
		lineOne.resize(16, '^');
		lineTwo.resize(16, '^');

		AsciiMessage message("dm");
		message += toAsciiDec(area + 1, 1);
		message += toAsciiDec(clear, 1);
		message += beepKeypad ? "1" : "0";
		message += toAsciiDec(displayTime, 5);
		message += lineOne;
		message += lineTwo;
		message += "00";

		// Send!
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::executePLCCommand(char houseCode, int unitCode, int functionCode, int extendedCode, int timeOn) { 
		// TODO: 'A' <= housecode <= 'P'
		// TODO: 0 <= unitCode <= 15
		// TODO: 0 <= functionCode <= 15
		// TODO: 0 <= extendedCode <= 99
		// TODO: 0 <= timeOn <= 9999

		AsciiMessage message("pc");
		message.push_back(houseCode);
		message += toAsciiDec(unitCode + 1, 2);
		message += toAsciiDec(functionCode + 1, 2);
		message += toAsciiDec(extendedCode, 2);
		message += toAsciiDec(timeOn, 4);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::enableControlOutput(int output, int seconds) {
		//TODO: 0 <= output <= 207
		//TODO: 0 <= seconds <= 65535

		AsciiMessage message("cn");
		message += toAsciiDec(output + 1, 3);
		message += toAsciiDec(seconds, 5);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::disableControlOutput(int output) {
		//TODO: 0 <= output <= 207

		AsciiMessage message("cf");
		message += toAsciiDec(output + 1, 3);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::setCustomValue(int index, uint16_t value) {
		//TODO: 0 <= index <= 19

		AsciiMessage message("cw");
		message += toAsciiDec(index + 1, 2);
		message += toAsciiDec(value, 5);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::setLogData(int logType, int eventType, int zoneNumber, int area) {
		// TODO: More testing, input restrictions
		AsciiMessage request("le");
		request += toAsciiDec(logType, 3);
		request += toAsciiDec(eventType, 3);
		request += toAsciiDec(zoneNumber + 1, 3);
		request += toAsciiDec(area + 1, 1);
		request += "00";
		cacheRequest(m1cache.okMessage, request, true, 0);
	}
	void M1AsciiAPI::setPLCState(char houseCode, int unitCode, bool state) { 
		// TODO: 0 <= unitCode <= 15
		AsciiMessage message(state ? "pn" : "pf");
		message.push_back(houseCode);
		message += toAsciiDec(unitCode + 1, 2);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::speakPhrase(int phraseIndex) {
		//TODO: replace with enum

		AsciiMessage message("sp");
		message += toAsciiDec(phraseIndex, 3);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::speakWord(int wordIndex) {
		//TODO: replace with enum
		AsciiMessage message("sw");
		message += toAsciiDec(wordIndex, 3);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::toggleControlOutput(int output) {
		//TODO: 0 <= output <= 207
		AsciiMessage message("ct");
		message += toAsciiDec(output + 1, 3);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::togglePLCState(char houseCode, int unitCode) {
		// TODO: 0 <= unitCode <= 15;
		AsciiMessage message("pt");
		message.push_back(houseCode);
		message += toAsciiDec(unitCode + 1, 2);
		message += "00";
		connection->Send(message.getTransmittable());
	}

#pragma endregion M1API Implementations
}