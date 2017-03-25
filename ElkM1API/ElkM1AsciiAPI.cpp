/*
	ElkM1AsciiAPI.cpp: Provides an implementation of the M1AsciiAPI class, using a high-performance function
	  table and blocks using mutexes with condition variables.
	@author Zach Jaggi
*/
#include "ElkM1AsciiAPI.h"
#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unordered_set>
#include <numeric>

#ifdef __CYGWIN__
namespace std {
	int stoi(const std::string& s) {
		return std::strtol(s.c_str(), 0, 10);
	}

	int stoi(const std::string& s, int pos, int base) {
		return std::strtol(s.c_str(), 0, base);
	}
}
#endif

namespace Elk {
#pragma region Static Helper Functions
	std::vector<char> M1AsciiAPI::toAsciiHex(int value, int length) {
		if ((value < 0) || (length < 0))
			throw std::invalid_argument("Argument out of allowed range.");

		std::vector<char> buff = std::vector<char>(length >= 5 ? length + 1 : 5);
		buff.resize(sprintf(&buff[0], "%0*.*X", length, length, value));
		if (buff.size() > length)
			buff.erase(buff.begin(), buff.begin() + (buff.size() - length));
		return buff;
	}

	std::vector<char> M1AsciiAPI::toAsciiDec(int value, int length) {
		if ((value < 0) || (length < 0))
			throw std::invalid_argument("Argument out of allowed range.");

		std::vector<char> buff = std::vector<char>(length >= 7 ? length + 1 : 7);
		buff.resize(sprintf(&buff[0], "%0*.*d", length, length, value));
		if (buff.size() > length)
			buff.erase(buff.begin(), buff.begin() + (buff.size() - length));
		return buff;
	}
	// Message checksum includes length bytes, but length bytes also account for having a checksum.
	std::vector<char> M1AsciiAPI::genChecksum(const std::vector<char>& message) {
		return toAsciiHex((std::accumulate(message.begin(), message.end(), 0) ^ 0xff) + 1, 2);
	}
	// Used for cache objects which are volatile.
	template <typename T>
	T M1AsciiAPI::cacheRequest(M1Monitor::cacheObject<T>& cacheObj, const AsciiMessage& request, bool ignoreCache, int timeoutMillis) {
		// Check if the cache is new enough and return that if it is
		if (!ignoreCache && (cacheObj.age() <= 3))
			return cacheObj.get();

		// If not, send our request and await the response.
		connection->Send(request.getTransmittable());

		return cacheObj.awaitNew(std::chrono::milliseconds(timeoutMillis));
	}
	// Used for cache objects which only change once per M1 programming.
	template <typename T>
	T M1AsciiAPI::cacheExistsRequest(M1Monitor::cacheObject<T>& cacheObj, const AsciiMessage& request) {
		// Check if the cache has been initialized
		if (cacheObj.isInitialized())
			return cacheObj.get();

		// If not, send our request and await the response.
		connection->Send(request.getTransmittable());

		return cacheObj.awaitNew(std::chrono::milliseconds(3000));
	}
#pragma endregion Static Helper Functions

#pragma region Protocol implementations

	// All incoming message handlers defined here. This uses an O(1) function table to handle new data, that is to say
	//   no matter how many commands are added, it will always take the same amount of time to find a command and handle
	//   the data. Message does not include length/checksum/crlf.
	M1AsciiAPI::M1AsciiAPI(std::shared_ptr<Elk::M1Connection> conn) : M1Monitor(conn) {
		
		// Arming Status Request
		handleMessageTable.emplace("AS", [this](std::string message) {
			std::shared_ptr<std::vector<ArmStatus>> newStatus(new std::vector<ArmStatus>(8));
			for (int i = 0; i < 8; i++) {
				newStatus->at(i).mode = (ArmMode)(message.at(2 + i) - '0');
				newStatus->at(i).isReady = (ArmUpMode)(message.at(10 + i) - '0');
				newStatus->at(i).alarm = (AlarmState)(message.at(18 + i) - '0');
			}
			m1cache.armStatus.set(*newStatus);

			// Create and detach the callback thread, self-cleans on exit
			if (onArmStatusChange)
				std::thread(&ArmStatusVectorCallback::run, onArmStatusChange, *newStatus).detach();
		});

		// Alarms by zone
		handleMessageTable.emplace("AZ", [this](std::string message) {
			std::vector<SZoneDefinition> zones(208);
			for (int i = 0; i < 208; i++) {
				zones[i].zd = (ZoneDefinition)(message.at(2 + i) - '0');
			}
			m1cache.zoneAlarms.set(zones);
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

		// Output Change Update
		// TODO: Test
		handleMessageTable.emplace("CC", [this](std::string message) {
			std::shared_ptr<std::vector<bool>> newStatus(new std::vector<bool>(m1cache.controlOutputs.get()));
			int zoneNumber = stoi(message.substr(2, 3)) - 1;
			newStatus->at(zoneNumber) = message.at(5) == '1';
			m1cache.controlOutputs.set(*newStatus);
			if (onOutputStatusChange)
				std::thread(&BoolVectorCallback::run, onOutputStatusChange, *newStatus).detach();
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

		// Control Output Status
		handleMessageTable.emplace("CS", [this](std::string message) {
			std::vector<bool> zones(208);
			for (int i = 0; i < 208; i++) {
				zones[i] = message.at(2 + i) == '1';
			}
			m1cache.controlOutputs.set(zones);
		});

		// User Code Changed
		handleMessageTable.emplace("CU", [this](std::string message) {
			int usercode = std::stoi(message.substr(2, 3));
			UserCodeSuccess userCodeSuccess = Elk::UserCodeSuccess::USERCODE_CHANGE_SUCCESSFUL;
			if (usercode == 0) { 
				userCodeSuccess = Elk::UserCodeSuccess::USERCODE_UNAUTHORIZED;
			}
			else if (usercode == 255) {
				userCodeSuccess = Elk::UserCodeSuccess::USERCODE_USER_DUPLICATE;
			}
			m1cache.userCodeChanged.set(userCodeSuccess);
		});

		// Counter value read
		handleMessageTable.emplace("CV", [this](std::string message) {
			// CVNNDDDDD00
			int counter = std::stoi(message.substr(2, 2)) - 1;
			int value = std::stoi(message.substr(4, 5));
			m1cache.counterValues[counter].set((uint16_t)value);
		});


		// Lighting status
		handleMessageTable.emplace("DS", [this](std::string message) {
			int index = stoi(message.substr(2, 3)) - 1;
			int value = stoi(message.substr(5, 2));
			m1cache.lightingStatus[index].set(value);
		});

		// Entry/Exit Time Data
		handleMessageTable.emplace("EE", [this](std::string message) {
			std::shared_ptr<EntryExitTimeData> newStatus(new EntryExitTimeData());
			newStatus->area = message.at(2) - '0' - 1;
			newStatus->timeDataType = (Elk::EntryExitTimeData::TimeDataType)(message.at(3) - '0');
			newStatus->timer1 = stoi(message.substr(4, 3));
			newStatus->timer2 = stoi(message.substr(7, 3));
			newStatus->armState = (Elk::ArmMode)(message.at(10) - '0');
			if (onEntryExitTimerChange)
				std::thread(&EntryExitTimeDataCallback::run, onEntryExitTimerChange, *newStatus).detach();
		});

		// User Code Validation
		handleMessageTable.emplace("IC", [this](std::string message) {
			int userCodeData = stoi(message.substr(2, 12), 0, 16);
			int userCodeNumber = stoi(message.substr(14, 3));
			int keypadNumber = stoi(message.substr(17, 2)) - 1;
			if (userCodeNumber == 0)
			{
				std::shared_ptr<InvalidUserCodeData> invalidCode(new InvalidUserCodeData());
				invalidCode->keypadNumber = keypadNumber;
				invalidCode->invalidUserCodeData = userCodeData;
				if (onInvalidUserCodeEntered)
					std::thread(&InvalidUserCodeDataCallback::run, onInvalidUserCodeEntered, *invalidCode).detach();
			}
			else {
				std::shared_ptr<ValidUserCodeData> validCode(new ValidUserCodeData());
				validCode->keypadNumber = keypadNumber;
				validCode->userCodeNumber = userCodeNumber;
				if (onValidUserCodeEntered)
					std::thread(&ValidUserCodeDataCallback::run, onValidUserCodeEntered, *validCode).detach();
			}
		});

		// RP disconnected
		handleMessageTable.emplace("IE", [this](std::string message) {
			// Create and detach the callback thread, self-cleans on exit
			if (onRPConnection)
				std::thread(&BoolCallback::run, onRPConnection, false).detach();
		});

		// Keypad Areas
		handleMessageTable.emplace("KA", [this](std::string message) {
			std::vector<int> areas(16);
			for (int i = 0; i < 16; i++) {
				areas[i] = message.at(2 + i) - '0' - 1;
			}
			m1cache.keypadAreas.set(areas);
		});

		// Keypad KeyChange Update
		handleMessageTable.emplace("KC", [this](std::string message) {
			std::shared_ptr<KeypadFkeyStatus> newStatus(new KeypadFkeyStatus);
			newStatus->keypadNumber = stoi(message.substr(2, 2)) - 1;
			newStatus->KeyPressed = (KeypadFkeyStatus::KeyID) stoi(message.substr(4, 2));
			for (int i = 0; i < 6; i++) {
				newStatus->illumination[i] = 
					(KeypadFkeyStatus::FkeyIllumination)(message.at(6 + i) - '0');
			}
			newStatus->codeRequiredForBypass = message.at(10) == '1';
			for (int i = 0; i < 8; i++)
			{
				newStatus->beepChimeMode[i] = message.at(11) - '0';
			}
			m1cache.keypadStatuses[newStatus->keypadNumber].set(*newStatus);

			if (onKeypadFkeyStatusChange)
				std::thread(&KeypadFkeyStatusCallback::run, onKeypadFkeyStatusChange, *newStatus).detach();
		});

		// Keypad function press TODO: Test
		handleMessageTable.emplace("KF", [this](std::string message) {
			std::vector<SChimeMode> chimeModes(8);
			for (int i = 0; i < 8; i++) {
				chimeModes[i].cm = (ChimeMode)(message.at(5 + i) - '0');
			}
			m1cache.chimeModes.set(chimeModes);
		});

		// Systems log data update
		handleMessageTable.emplace("LD", [this](std::string message) {
			std::shared_ptr<Elk::LogEntry> newEntry(new Elk::LogEntry());
			newEntry->event = stoi(message.substr(2, 4));
			newEntry->area = message.at(9) - '0' - 1;
			newEntry->eventSubjectNumber = stoi(message.substr(6, 3));
			newEntry->hour = stoi(message.substr(10, 2));
			newEntry->minute = stoi(message.substr(12, 2));
			newEntry->month = stoi(message.substr(14, 2));
			newEntry->day = stoi(message.substr(16, 2));
			newEntry->index = stoi(message.substr(18, 3)) - 1;
			newEntry->dayOfWeek = (Elk::Weekday)(message.at(21) - '0');
			newEntry->year = stoi(message.substr(22, 2));

			m1cache.logData[newEntry->index].set(*newEntry);

			if (onLogDataUpdate)
				std::thread(&LogEntryCallback::run, onLogDataUpdate, *newEntry).detach();
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

		// Lighting Change Update
		handleMessageTable.emplace("PC", [this](std::string message) {
			char houseCode = message.at(2);
			int unitCode = stoi(message.substr(3, 2));
			int lightLevel = stoi(message.substr(5, 2));
			if (unitCode == 0) {
				std::shared_ptr<Elk::LightingData> lightingData(new Elk::LightingData());
				lightingData->houseCode = houseCode;
				lightingData->unitCode = unitCode - 1;
				lightingData->lightLevel = (lightLevel > 1) ? (lightLevel / 100) : lightLevel;
				if (onLightingDataUpdate)
					std::thread(&LightingDataCallback::run, onLightingDataUpdate, *lightingData).detach();
			}
			else {
				std::shared_ptr<Elk::X10Data> x10Data(new Elk::X10Data());
				x10Data->houseCode = houseCode;
				x10Data->x10 = (Elk::X10Data::X10)lightLevel;
				if (onX10DataUpdate)
					std::thread(&X10DataCallback::run, onX10DataUpdate, *x10Data).detach();
			}
		});

		// Returned PLC status
		handleMessageTable.emplace("PS", [this](std::string message) {
			std::vector<int> lightingLevels(64);
			int bank = message.at(2) - '0';
			for (int i = 0; i < 64; i++) {
				lightingLevels[i] = message.at(3 + i) - '0';
			}
			m1cache.plcStatus[bank].set(lightingLevels);
		});

		// RP connected
		handleMessageTable.emplace("RP", [this](std::string message) {
			// Invalidate the cache, causing currently blocked calls to throw exceptions
			m1cache.invalidate();
			// Create and detach the callback thread, self-cleans on exit
			if (onRPConnection)
				std::thread(&BoolCallback::run, onRPConnection, true).detach();
		});

		// RTC Data
		handleMessageTable.emplace("RR", [this](std::string message) {
			RTCData rtc;
			rtc.seconds = stoi(message.substr(2, 2));
			rtc.minutes = stoi(message.substr(4, 2));
			rtc.hours = stoi(message.substr(6, 2));
			rtc.weekday = (Weekday)(message.at(8) - '0');
			rtc.day = stoi(message.substr(9, 2));
			rtc.month = stoi(message.substr(11, 2));
			rtc.year = stoi(message.substr(13, 2)) + 2000;
			rtc.twelveHourClock = message.at(16) == '1';
			rtc.dayBeforeMonth = message.at(17) == '1';
			m1cache.rtcData.set(rtc);
		});

		// Strings
		handleMessageTable.emplace("SD", [this](std::string message) {
			TextDescriptionType tdt = TextDescriptionType(stoi(message.substr(2, 2)));
			int index = stoi(message.substr(4, 3)) - 1;
			std::string desc = message.substr(7, 16);
			// Normally std::isspace would be good practice, but we're using ASCII so there's only the one.
			// TODO: Error setting blank spaces?
			desc.erase(std::find_if(desc.rbegin(), desc.rend(), [](char c) { return c != ' '; }).base(), desc.end());
			switch (tdt) {
			case TEXT_ZoneName:
				return m1cache.ZoneNames[index].set(desc);
			case TEXT_AreaName:
				return m1cache.AreaNames[index].set(desc);
			case TEXT_UserName:
				return m1cache.UserNames[index].set(desc);
			case TEXT_KeypadName:
				return m1cache.KeypadNames[index].set(desc);
			case TEXT_OutputName:
				return m1cache.OutputNames[index].set(desc);
			case TEXT_TaskName:
				return m1cache.TaskNames[index].set(desc);
			case TEXT_TelephoneName:
				return m1cache.TelephoneNames[index].set(desc);
			case TEXT_LightName:
				return m1cache.LightNames[index].set(desc);
			case TEXT_AlarmDurationName:
				return m1cache.AlarmDurationNames[index].set(desc);
			case TEXT_CustomSettings:
				return m1cache.CustomSettingNames[index].set(desc);
			case TEXT_CounterName:
				return m1cache.CounterNames[index].set(desc);
			case TEXT_ThermostatName:
				return m1cache.ThermostatNames[index].set(desc);
			case TEXT_FKEY1:
				return m1cache.FKEY1s[index].set(desc);
			case TEXT_FKEY2:
				return m1cache.FKEY2s[index].set(desc);
			case TEXT_FKEY3:
				return m1cache.FKEY3s[index].set(desc);
			case TEXT_FKEY4:
				return m1cache.FKEY4s[index].set(desc);
			case TEXT_FKEY5:
				return m1cache.FKEY5s[index].set(desc);
			case TEXT_FKEY6:
				return m1cache.FKEY6s[index].set(desc);
			case TEXT_AudioZoneName:
				return m1cache.AudioZoneNames[index].set(desc);
			case TEXT_AudioSourceName:
				return m1cache.AudioSourceNames[index].set(desc);
			}
			if (onDebugOutput)
			{
				std::string output = "Error parsing message: " + message + "\n";
				std::thread(&StringCallback::run, onDebugOutput, output).detach();
			}
		});

		// System Trouble Status
		handleMessageTable.emplace("SS", [this](std::string message) {  
			SystemTroubleStatus sts;
			sts.ACFail = message.at(2) == '1';
			sts.boxTamperZoneNumber = message.at(3) - '1';
			sts.communicationError = message.at(4) == '1';
			sts.EEPROMError = message.at(5) == '1';
			sts.lowBatteryControl = message.at(6) == '1';
			sts.transmitterLowBatteryZoneNumber = message.at(7) - '1';
			sts.overCurrent = message.at(8) == '1';
			sts.telephoneFault = message.at(9) == '1';
			sts.output2 = message.at(11) == '1';
			sts.missingKeypad = message.at(12) == '1';
			sts.zoneExpander = message.at(13) == '1';
			sts.outputExpander = message.at(14) == '1';
			sts.RPRemoteAccess = message.at(16) == '1';
			sts.commonAreaNotArmed = message.at(18) == '1';
			sts.flashMemoryError = message.at(19) == '1';
			sts.securityAlertZoneNumber = message.at(20) - '1';
			sts.serialPortExpander = message.at(21) == '1';
			sts.lostTransmitterZoneNumber = message.at(22) - '1';
			sts.GESmokeCleanMe = message.at(23) == '1';
			sts.ethernet = message.at(24) == '1';
			sts.displayMessageKeypadLine1 = message.at(33) == '1';
			sts.displayMessageKeypadLine2 = message.at(34) == '1';
			sts.fireTroubleZoneNumber = message.at(35) - '1';
			m1cache.systemTroubleStatus.set(sts);
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

		// Omnistat 2 reply
		handleMessageTable.emplace("T2", [this](std::string message) {
			std::vector<char> reply;
			for (int i = 0; i < 16; i++)
				reply.push_back(stoi(message.substr(2 + (i * 2)), 0, 16));
			m1cache.omniStat2Reply.set(reply);
		});

		// Task Change Update
		handleMessageTable.emplace("TC", [this](std::string message) {
			int taskNumber = stoi(message.substr(2, 3)) - 1;
			if (onTaskChangeUpdate)
				std::thread(&IntCallback::run, onTaskChangeUpdate, taskNumber).detach();
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

		// User code areas
		handleMessageTable.emplace("UA", [this](std::string message) {
			UserCodeAccess uca;
			uca.validAreas = stoi(message.substr(8, 2), 0, 16);
			uca.codetype = (UserCodeAccess::CodeType)(message.at(18) - '0');
			uca.usesCelcius = message.at(20) == 'C';
			m1cache.userCodeAccess.set(uca);
		});

		// Version number
		handleMessageTable.emplace("VN", [this](std::string message) {
			std::vector<int> vn(3);
			std::vector<int> xvn(3);
			for (int i = 0; i < 3; i++) {
				vn[i] = stoi(message.substr(2 + (2 * i), 2), 0, 16);
				xvn[i] = stoi(message.substr(8 + (2 * i), 2), 0, 16);
			}
			m1cache.M1VersionNumber.set(vn);
			m1cache.XEPVersionNumber.set(xvn);
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

		// Zone Change Update
		handleMessageTable.emplace("ZC", [this](std::string message) {
			std::shared_ptr<ZoneState> zoneState(new ZoneState());
			int bitfield = message.at(5);
			bitfield -= ((bitfield >= 'A') ? ('A' - 10) : '0');
			zoneState->physicalState = (PhysicalZoneState)(bitfield & 0x3);
			zoneState->logicalState = (LogicalZoneState)(bitfield >> 2);

			std::vector<ZoneState> zones = getZoneStatuses();
			int zoneNumber = stoi(message.substr(2, 3)) - 1;
			zones.at(zoneNumber) = *zoneState;
			m1cache.zoneStatus.set(zones);

			if (onZoneChangeUpdate)
				std::thread(&ZoneStateCallback::run, onZoneChangeUpdate, *zoneState).detach();
		});

		// Definitions by zone
		handleMessageTable.emplace("ZD", [this](std::string message) {
			std::vector<SZoneDefinition> zones(208);
			for (int i = 0; i < 208; i++) {
				zones[i].zd = (ZoneDefinition)(message.at(2 + i) - '0');
			}
			m1cache.zoneDefinitions.set(zones);
		});

		// Zone Partitions
		handleMessageTable.emplace("ZP", [this](std::string message) {
			std::vector<int> zones(208);
			for (int i = 0; i < 208; i++) {
				zones[i] = message.at(2 + i) - '0' - 1;
			}
			m1cache.zonePartitions.set(zones);
		});

		// Zone Statuses
		handleMessageTable.emplace("ZS", [this](std::string message) {
			std::vector<ZoneState> zones(208);
			for (int i = 0; i < 208; i++) {
				// Ascii to Int
				int bitfield = message.at(2 + i) - ((message.at(2 + i) >= 'A') ?
					('A' - 10):
					'0');
				zones[i] = { (PhysicalZoneState)(bitfield & 0x3),
					(LogicalZoneState)(bitfield >> 2) };
			}
			m1cache.zoneStatus.set(zones);
		});

		// Zone Voltage
		handleMessageTable.emplace("ZV", [this](std::string message) {
			int index = std::stoi(message.substr(2, 3)) - 1;
			int value = std::stoi(message.substr(5, 3));
			m1cache.zoneVoltage[index].set(
				((float)value) / 10
				);
		});
	}

	std::vector<char> M1AsciiAPI::cutMessage(std::vector<char>& buffer) {
		auto newlineItr = std::find(buffer.begin(), buffer.end(), '\n');

		// If not found, return our blank buffer immediately
		if (newlineItr == buffer.end())
			return std::vector<char>();
		
		// Move (efficiently) the buffer contents into the result, and erase it from the buffer in one operation
		std::vector<char> result(buffer.begin(), newlineItr + 1);
		buffer.erase(buffer.begin(), newlineItr + 1);
		return result;
	}

	void M1AsciiAPI::handleMessage(std::vector<char> message) {
		if (message.size() < 4) {
			message.push_back('\0');
			if (onDebugOutput)
			{
				std::string output = "Caught malformed message: \"" + std::string(&message[0]) + "\"\n";
				std::thread(&StringCallback::run, onDebugOutput, output).detach();
			}
			return;
		}

		// Catch "OK\r\n" TODO: Checked for in every message, perhaps another way?
		if (!memcmp(&message[0], "OK\r\n", 4)) {
			m1cache.okMessage.set(true);
			return;
		}

		// Find identifying bytes (3 and 4)
		std::string identifier(message.begin() + 2, message.begin() + 4);

			// Call appropriate function
		auto itr = handleMessageTable.find(identifier);
		if(itr != handleMessageTable.end()) {
			itr->second(AsciiMessage::fromTransmission(message).to_string());
		}
		else {
			message.push_back('\0');
			if (onDebugOutput)
			{
				std::string output = "No handler for message: \"" + std::string(&message[0]) + "\"\n";
				std::thread(&StringCallback::run, onDebugOutput, output).detach();
			}
		}
	}

#pragma endregion Protocol implementations

#pragma region M1API Implementations
	bool M1AsciiAPI::versionAtLeast(int major, int minor, int release) {
		int minVersion[] = { major, minor, release };
		return memcmp(&getM1VersionNumber()[0], &minVersion, 3) >= 0;
	}

	std::vector<int> M1AsciiAPI::getConfiguredZones() {
		const auto& zdef = getZoneDefinitions();
		std::vector<int> configured;
		for (int i = 0; i < zdef.size(); i++) {
			if (zdef[i].zd != ZONEDEF_DISABLED) {
				configured.push_back(i);
			}
		}
		return configured;
	}

	std::vector<int> M1AsciiAPI::getConfiguredAreas() {
		std::unordered_set<int> areas;
		std::vector<int> zoneAreas = getZonePartitions();
		std::vector<int> keypadAreas = getKeypadAreas();
		areas.insert(zoneAreas.begin(), zoneAreas.end());
		areas.insert(keypadAreas.begin(), keypadAreas.end());
		areas.erase(-1);
		std::vector<int> ret(areas.begin(), areas.end());
		std::sort(ret.begin(), ret.end());
		return ret;
	}

	std::vector<int> M1AsciiAPI::getConfiguredKeypads() {
		std::vector<int> kpa = getKeypadAreas();
		std::vector<int> configured;
		for (int i = 0; i < kpa.size(); i++) {
			if (kpa[i] != -1) {
				configured.push_back(i);
			}
		}
		return configured;
	}

	std::vector<std::pair<int, TemperatureDevice>> M1AsciiAPI::getConfiguredTempDevices() {
		std::vector<std::pair<int, TemperatureDevice>> configured;
		for (int type = Elk::TEMPDEVICE_ZONE; type < Elk::TEMPDEVICE_THERMOSTAT; type++){
			const auto& temp = getTemperatures(Elk::TemperatureDevice(type));
			for (int i = 0; i < temp.size(); i++) {
				if (temp[i] != INT_MIN) {
					configured.emplace_back(i, (TemperatureDevice)type);
				}
			}
		}
		return configured;
	}

	// TODO: Function to intelligently collect names: BUGGY! Doesn't work well over the Proxy.
	// Should be somehow prioritized below regular send/rcv requests.
	void M1AsciiAPI::collectNames(TextDescriptionType type) {
		std::vector<int> indexes;
		cacheObject<std::string> *beginItr;
		cacheObject<std::string> *endItr;

		switch (type) {
		case TEXT_AreaName:
			// Request all area names 
			indexes = getConfiguredAreas();
			beginItr = std::begin(m1cache.AreaNames);
			endItr = std::end(m1cache.AreaNames);
			break;
		case TEXT_ZoneName:
			// Request all area names 
			indexes = getConfiguredZones();
			beginItr = std::begin(m1cache.ZoneNames);
			endItr = std::end(m1cache.ZoneNames);
			break;
		case TEXT_KeypadName:
			// Request all area names 
			indexes = getConfiguredKeypads();
			beginItr = std::begin(m1cache.KeypadNames);
			endItr = std::end(m1cache.KeypadNames);
			break;
		case TEXT_OutputName:
			indexes = std::vector<int>(64);
			for (int i = 0; i < indexes.size(); i++)
				indexes[i] = i;
			beginItr = std::begin(m1cache.OutputNames);
			endItr = std::end(m1cache.OutputNames);
			break;
		default:
			throw std::runtime_error("Not imlemented.");
		}
		// If all are initialized, quit early
		if (std::all_of(beginItr, endItr, [](cacheObject<std::string>& obj) {
			return obj.isInitialized();
		})) {
			return;
		}
		
		for (int index : indexes) {
			AsciiMessage message("sd");
			message += toAsciiDec(type, 2);
			message += toAsciiDec(index + 1, 3);
			message += "00";
			connection->Send(message.getTransmittable());
		}
		// Timeout on the entire block
		try {
			if (!beginItr[indexes.back()].isInitialized())
				beginItr[indexes.back()].awaitNew(std::chrono::milliseconds(1500));
		}
		catch (...)
		{
		}
		// Fill untouched caches with blanks
		for (auto& block = beginItr; block != endItr; std::advance(block, 1)){
			if (!block->isInitialized())
				block->set("");
		}
	}
	
	AudioData M1AsciiAPI::getAudioData(int audioZone) { 
		// TODO: Can't test without something to test against.
		if ( (audioZone < 0) || (audioZone >= 16) ) {
			throw std::invalid_argument("Argument out of allowed range.");
		}
		AsciiMessage message("ca");
		message += toAsciiDec(audioZone + 1, 2);
		message += "00";
		return cacheRequest(m1cache.audioData[audioZone], message, false, 1500);
	}
	bool M1AsciiAPI::setAreaBypass(int area, std::string pinCode, bool bypassed) {
		if ((area < 0) || (area >= 8)) {
			throw std::invalid_argument("Argument out of allowed range.");
		}
		// TODO: Test this more!
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
		if ((zone < 0) || (zone >= 208)) {
			throw std::invalid_argument("Argument out of allowed range.");
		}
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
		if (!versionAtLeast(4, 2, 8)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		if ((zone < 0) || (zone >= 208)) {
			throw std::invalid_argument("Argument out of allowed range.");
		}
		AsciiMessage message("zv");
		message += toAsciiDec(zone + 1, 3);
		message += "00";
		return cacheRequest<float>(m1cache.zoneVoltage[zone], message, true, 0);
	}
	int M1AsciiAPI::getLightingStatus(int device) {
		if (!versionAtLeast(4, 3, 9)) 
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		if ((device < 0) || (device >= 256))
			throw std::invalid_argument("Argument out of allowed range.");
		AsciiMessage message("ds");
		message += toAsciiDec(device + 1, 3);
		message += "00";
		return cacheRequest<int>(m1cache.lightingStatus[device], message, true, 0);
	}
	int M1AsciiAPI::getTemperature(TemperatureDevice type, int device) {
		if ((device < 0) || (device >= 16))
			throw std::invalid_argument("Argument out of allowed range.");

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
	std::vector<int> M1AsciiAPI::getTemperatures(TemperatureDevice type) {
		if (!versionAtLeast(4, 3, 4))
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		std::vector<int> reply(16);
		// There's three different ways to get temperature data. Select the best for the task.
		switch (type)
		{
		case TEMPDEVICE_KEYPAD:
			for (int i = 0; i < 16; i++) {
				reply[i] = cacheRequest(m1cache.keypadTemperatures[i], (const AsciiMessage&)"lw00", false, 0);
			}
			return reply;
		case TEMPDEVICE_ZONE:
			for (int i = 0; i < 16; i++) {
				reply[i] = cacheRequest(m1cache.zoneTemperatures[i], (const AsciiMessage&)"lw00", false, 0);
			}
			return reply;
		case TEMPDEVICE_THERMOSTAT:
			for (int i = 0; i < 16; i++) {
				reply[i] = getTemperature(type, i);
			}
			return reply;
		}
		return std::vector<int>();
	}
	KeypadFkeyStatus M1AsciiAPI::getKeypadFkeyStatus(int keypad) {
		if ((keypad < 0) || (keypad >= 16))
			throw std::invalid_argument("Argument out of allowed range.");

		AsciiMessage message("kc");
		message += toAsciiDec(keypad + 1, 2);
		message += "00";
		return cacheRequest(m1cache.keypadStatuses[keypad], message, true, 0);
	}
	LogEntry M1AsciiAPI::getLogData(int index) {
		if ((index < 0) || (index >= 510))
			throw std::invalid_argument("Argument out of allowed range.");

		AsciiMessage request("ld");
		request += toAsciiDec(index + 1, 3);
		request += "00";
		return cacheRequest(m1cache.logData[index], request, true, 0);
	}
	std::vector<LogEntry> M1AsciiAPI::getLogs() {
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
		return logs;
	}
	std::vector<int> M1AsciiAPI::getPLCStatus(int bank) {
		if ((bank < 0) || (bank >= 4))
			throw std::invalid_argument("Argument out of allowed range.");

		AsciiMessage request("ps");
		request += toAsciiDec(bank, 1);
		request += "00";
		return cacheRequest(m1cache.plcStatus[bank], request, true, 0);
	}
	RTCData M1AsciiAPI::getRTCData() { 
		if (!versionAtLeast(4, 3, 2)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		return cacheRequest(m1cache.rtcData, (const AsciiMessage&)"rr00", true, 0);
	}
	RTCData M1AsciiAPI::setRTCData(RTCData newData) { 
		if (!versionAtLeast(4, 3, 2)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		AsciiMessage message("rw");
		message += toAsciiDec(newData.seconds, 2);
		message += toAsciiDec(newData.minutes, 2);
		message += toAsciiDec(newData.hours, 2);
		message += toAsciiDec(newData.weekday, 1);
		message += toAsciiDec(newData.day, 2);
		message += toAsciiDec(newData.month, 2);
		message += toAsciiDec(newData.year % 100, 2);
		message += "00";
		return cacheRequest(m1cache.rtcData, message, true, 0);
	}
	std::vector<ArmStatus> M1AsciiAPI::getArmStatus() { 
		return cacheRequest(m1cache.armStatus, (const AsciiMessage&)"as00", true, 0);
	}
	std::vector<bool> M1AsciiAPI::getControlOutputs() { 
		return cacheRequest(m1cache.controlOutputs, (const AsciiMessage&)"cs00", true, 0);
	}
	std::vector<SChimeMode> M1AsciiAPI::pressFunctionKey(int keypad, FKEY key) { 
		if (!versionAtLeast(4, 2, 5)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		if ((keypad < 0) || (keypad >= 16))
			throw std::invalid_argument("Argument out of allowed range.");

		AsciiMessage message("kf");
		message += toAsciiDec(keypad + 1, 2);
		switch (key) {
		case FKEY_STAR:
			if (!versionAtLeast(4, 2, 6)) {
				throw std::runtime_error("Star key unsupported by M1 Firmware version < 4.2.6.");
			}
			message += "*";
			break;
		case FKEY_CHIME:
			if (!versionAtLeast(4, 3, 2)) {
				throw std::runtime_error("Chime key unsupported by M1 Firmware version < 4.3.2.");
			}
			message += "C";
			break;
		default:
			message += toAsciiDec((int)key, 1);
		}
		message += "00";
		return cacheRequest(m1cache.chimeModes, message, true, 0);
	}
	std::vector<int> M1AsciiAPI::getKeypadAreas() { 
		if (!versionAtLeast(4, 2, 5)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		return cacheExistsRequest(m1cache.keypadAreas, (const AsciiMessage&)"ka00");
	}
	std::vector<int> M1AsciiAPI::getZonePartitions() {
		return cacheExistsRequest(m1cache.zonePartitions, (const AsciiMessage&)"zp00");
	}
	std::vector<int> M1AsciiAPI::getM1VersionNumber() { 
		return cacheExistsRequest(m1cache.M1VersionNumber, (const AsciiMessage&)"vn00");
	}
	std::vector<int> M1AsciiAPI::getXEPVersionNumber() {
		return cacheExistsRequest(m1cache.XEPVersionNumber, (const AsciiMessage&)"vn00");
	}
	std::vector<uint16_t> M1AsciiAPI::getCustomValues() { 
		std::vector<uint16_t> reply(20);
		for (int i = 0; i < 20; i++) {
			reply[i] = cacheRequest(m1cache.customValues[i], (const AsciiMessage&)"cp00", false, 0);
		}
		return reply;
	}
	std::vector<SZoneDefinition> M1AsciiAPI::getZoneAlarms() { 
		if (!versionAtLeast(4, 3, 9)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		return cacheRequest(m1cache.zoneAlarms, (const AsciiMessage&)"az00", true, 0);
	}
	std::vector<SZoneDefinition> M1AsciiAPI::getZoneDefinitions() { 
		if (!versionAtLeast(4, 2, 6)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		return cacheExistsRequest(m1cache.zoneDefinitions, (const AsciiMessage&)"zd00");
	}
	std::vector<ZoneState> M1AsciiAPI::getZoneStatuses() { 
		return cacheRequest(m1cache.zoneStatus, (const AsciiMessage&)"zs00", true, 0);
	}
	std::string M1AsciiAPI::getTextDescription(TextDescriptionType type, int index) { 
		AsciiMessage message("sd");
		message += toAsciiDec(type, 2);
		message += toAsciiDec(index + 1, 3);
		message += "00";
		switch (type)
		{
		case TEXT_ZoneName:
			if ((index < 0) || (index >= 208))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.ZoneNames[index], message);
		case TEXT_AreaName:
			if ((index < 0) || (index >= 8))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.AreaNames[index], message);
		case TEXT_UserName:
			if ((index < 0) || (index >= 199))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.UserNames[index], message);
		case TEXT_KeypadName:
			if ((index < 0) || (index >= 16))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.KeypadNames[index], message);
		case TEXT_OutputName:
			if ((index < 0) || (index >= 208))
				throw std::invalid_argument("Argument out of allowed range.");
			if (index >= 64)
				return "";
			return cacheExistsRequest(m1cache.OutputNames[index], message);
		case TEXT_TaskName:
			if ((index < 0) || (index >= 32))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.TaskNames[index], message);
		case TEXT_TelephoneName:
			if ((index < 0) || (index >= 8))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.TelephoneNames[index], message);
		case TEXT_LightName:
			if ((index < 0) || (index >= 256))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.LightNames[index], message);
		case TEXT_AlarmDurationName:
			if ((index < 0) || (index >= 12))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.AlarmDurationNames[index], message);
		case TEXT_CustomSettings:
			if ((index < 0) || (index >= 20))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.CustomSettingNames[index], message);
		case TEXT_CounterName:
			if ((index < 0) || (index >= 64))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.CounterNames[index], message);
		case TEXT_ThermostatName:
			if ((index < 0) || (index >= 16))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.ThermostatNames[index], message);
		case TEXT_FKEY1:
			if ((index < 0) || (index >= 16))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.FKEY1s[index], message);
		case TEXT_FKEY2:
			if ((index < 0) || (index >= 16))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.FKEY2s[index], message);
		case TEXT_FKEY3:
			if ((index < 0) || (index >= 16))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.FKEY3s[index], message);
		case TEXT_FKEY4:
			if ((index < 0) || (index >= 16))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.FKEY4s[index], message);
		case TEXT_FKEY5:
			if ((index < 0) || (index >= 16))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.FKEY5s[index], message);
		case TEXT_FKEY6:
			if ((index < 0) || (index >= 16))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.FKEY6s[index], message);
		case TEXT_AudioZoneName:			
			if ((index < 0) || (index >= 18))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.AudioZoneNames[index], message);
		case TEXT_AudioSourceName:
			if ((index < 0) || (index >= 19))
				throw std::invalid_argument("Argument out of allowed range.");
			return cacheExistsRequest(m1cache.AudioSourceNames[index], message);
		}
		throw std::runtime_error("TextDescriptionType not defined.");
	}
	std::vector<char> M1AsciiAPI::getOmnistat2Data(std::vector<char> request) { 
		if (!versionAtLeast(5, 1, 9)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		// TODO: Unable to test yet.
		AsciiMessage message("t2");
		for (auto byte : request)
			message += toAsciiHex(byte, 2);
		message += "00";
		return cacheRequest(m1cache.omniStat2Reply, message, true, 0);
	}
	SystemTroubleStatus M1AsciiAPI::getSystemTroubleStatus() {
		if (!versionAtLeast(4, 5, 4)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		return cacheRequest(m1cache.systemTroubleStatus, (const AsciiMessage&)"ss00", true, 0);
	}
	ThermostatData M1AsciiAPI::getThermostatData(int index) { 
		if (!versionAtLeast(4, 2, 6)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		if ((index < 0) || (index >= 16))
			throw std::invalid_argument("Argument out of allowed range.");
		AsciiMessage request("tr");
		request += toAsciiDec(index + 1, 2);
		request += "00";
		return cacheRequest(m1cache.thermostatData[index], request, false, 0);
	}
	ThermostatData M1AsciiAPI::setThermostatData(int index, int value, int element) {
		// TODO: replace element with enum
		if ((index < 0) || (index >= 16))
			throw std::invalid_argument("Argument out of allowed range.");
		AsciiMessage request("ts");
		request += toAsciiDec(index + 1, 2);
		request += toAsciiDec(value, 2);
		request += toAsciiDec(element, 1);
		request += "00";
		return cacheRequest(m1cache.thermostatData[index], request, true, 0);
	}
	uint16_t M1AsciiAPI::getCounterValue(int counter) {
		if (!versionAtLeast(4, 1, 11)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		if ((counter < 0) || (counter >= 64))
			throw std::invalid_argument("Argument out of allowed range.");
		AsciiMessage message("cv");
		message += toAsciiDec(counter + 1, 2);
		message += "00";
		return cacheRequest(m1cache.counterValues[counter], message, true, 0);
	}
	uint16_t M1AsciiAPI::getCustomValue(int index) {
		if ((index < 0) || (index >= 20))
			throw std::invalid_argument("Argument out of allowed range.");
		AsciiMessage message("cr");
		message += toAsciiDec(index + 1, 2);
		message += "00";
		return cacheRequest(m1cache.customValues[index], message, true, 0);
	}
	uint16_t M1AsciiAPI::setCounterValue(int counter, uint16_t value) { 
		if (!versionAtLeast(4, 1, 11)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		if ((counter < 0) || (counter >= 64))
			throw std::invalid_argument("Argument out of allowed range.");
		AsciiMessage message("cx");
		message += toAsciiDec(counter + 1, 2);
		message += toAsciiDec(value, 5);
		message += "00";

		// If we have a timeout defined, try to use that, and rethrow exception if we wait too long
		return cacheRequest(m1cache.counterValues[counter], message, true, 0);
	}
	UserCodeAccess M1AsciiAPI::getUserCodeAccess(std::string userCode) { 
		if (!versionAtLeast(4, 2, 5)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		AsciiMessage message("ua");
		if (userCode.length() == 4) {
			message += "00";
		}
		message += userCode;
		message += "00";
		return cacheRequest(m1cache.userCodeAccess, message, true, 0);
	}
	UserCodeSuccess M1AsciiAPI::requestChangeUserCode(int user, std::string authCode, std::string newUserCode, uint8_t areaMask) { 
		if (!versionAtLeast(4, 3, 9)) {
			throw std::runtime_error("Call unsupported by M1 Firmware version.");
		}
		if (authCode.length() < 6)
			authCode.insert(0, 6 - authCode.length(), '0');
		if (newUserCode.length() < 6)
			newUserCode.insert(0, 6 - newUserCode.length(), '0');
		AsciiMessage request("cu");
		request += toAsciiDec(user + 1, 3);
		for (char c : authCode) {
			request += toAsciiDec(c - '0', 2);
		}
		for (char c : newUserCode) {
			request += toAsciiDec(c - '0', 2);
		}
		request += toAsciiHex(areaMask, 2);
		request += "00";
		return cacheRequest(m1cache.userCodeChanged, request, true, 0); // TODO: Block on concurrent hashmap or something
	}
	void M1AsciiAPI::activateTask(int taskNumber) {
		if ((taskNumber < 0) || (taskNumber >= 32))
			throw std::invalid_argument("Argument out of allowed range.");
		AsciiMessage message("tn");
		message += toAsciiDec(taskNumber + 1, 3);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::armDisarm(int area, ArmMode mode, std::string userCode) {
		if ((userCode.size() != 4) && (userCode.size() != 6))
			throw std::invalid_argument("Argument out of allowed range.");
		if ((area < 0) || (area >= 8))
			throw std::invalid_argument("Argument out of allowed range.");

		// userCode = pad left with 0s if 4 digits
		if (userCode.length() < 6)
			userCode.insert(0, 6 - userCode.length(), '0');
		AsciiMessage message("a");

		// ArmLevel TODO: 7, 8 are only in m1ver > 4.2.8
		if ((int(mode) >= 7) && !versionAtLeast(4, 2, 8))
		{
			throw std::invalid_argument("Argument unsupported by firmware version.");
		}
		message += toAsciiDec(mode, 1);
		message += toAsciiDec(area + 1, 1);
		message += userCode;
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::displayLCDText(int area, clearMethod clear, bool beepKeypad, uint16_t displayTime, std::string text) {
		if ((area < 0) || (area >= 8))
			throw std::invalid_argument("Argument out of allowed range.");

		text.resize(32, '^');
		displayLCDText(area, clear, beepKeypad, displayTime, text.substr(0, 16), text.substr(16, 16));
	}
	void M1AsciiAPI::displayLCDText(int area, clearMethod clear, bool beepKeypad, uint16_t displayTime, std::string lineOne, std::string lineTwo) { 
		if ((area < 0) || (area >= 8))
			throw std::invalid_argument("Argument out of allowed range.");

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
		if ((houseCode < 'A') || (houseCode > 'P'))
			throw std::invalid_argument("Argument out of allowed range.");
		if ((unitCode < 0) || (unitCode >= 16))
			throw std::invalid_argument("Argument out of allowed range.");
		if ((functionCode < 0) || (functionCode >= 16))
			throw std::invalid_argument("Argument out of allowed range.");
		if ((extendedCode < 0) || (extendedCode > 99))
			throw std::invalid_argument("Argument out of allowed range.");
		if ((timeOn < 0) || (timeOn > 9999))
			throw std::invalid_argument("Argument out of allowed range.");

		AsciiMessage message("pc");
		message.push_back(houseCode);
		message += toAsciiDec(unitCode + 1, 2);
		message += toAsciiDec(functionCode + 1, 2);
		message += toAsciiDec(extendedCode, 2);
		message += toAsciiDec(timeOn, 4);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::enableControlOutput(int output, uint16_t seconds) {
		if ((output < 0) || (output >= 208))
			throw std::invalid_argument("Argument out of allowed range.");

		AsciiMessage message("cn");
		message += toAsciiDec(output + 1, 3);
		message += toAsciiDec(seconds, 5);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::disableControlOutput(int output) {
		if ((output < 0) || (output >= 208))
			throw std::invalid_argument("Argument out of allowed range.");

		AsciiMessage message("cf");
		message += toAsciiDec(output + 1, 3);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::setCustomValue(int index, uint16_t value) {
		if ((index < 0) || (index >= 20))
			throw std::invalid_argument("Argument out of allowed range.");

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
		if ((unitCode < 0) || (unitCode >= 16))
			throw std::invalid_argument("Argument out of allowed range.");
		AsciiMessage message(state ? "pn" : "pf");
		message.push_back(houseCode);
		message += toAsciiDec(unitCode + 1, 2);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::speakPhrase(SirenPhrase phrase) {
		AsciiMessage message("sp");
		message += toAsciiDec((int)phrase, 3);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::speakWord(SirenWord word) {
		AsciiMessage message("sw");
		message += toAsciiDec((int)word, 3);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::toggleControlOutput(int output) {
		if ((output < 0) || (output >= 208))
			throw std::invalid_argument("Argument out of allowed range.");
		AsciiMessage message("ct");
		message += toAsciiDec(output + 1, 3);
		message += "00";
		connection->Send(message.getTransmittable());
	}
	void M1AsciiAPI::togglePLCState(char houseCode, int unitCode) {
		if ((unitCode < 0) || (unitCode >= 16))
			throw std::invalid_argument("Argument out of allowed range.");
		AsciiMessage message("pt");
		message.push_back(houseCode);
		message += toAsciiDec(unitCode + 1, 2);
		message += "00";
		connection->Send(message.getTransmittable());
	}

#pragma endregion M1API Implementations
}
