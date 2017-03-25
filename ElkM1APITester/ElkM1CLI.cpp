// ElkM1APITester.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <mutex>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <memory>
#include <map>
#include "ElkM1API.h"
#include "ElkM1AsciiAPI.h"
#include "ElkM1TCP.h"

std::mutex mtx;
bool sigExit = false;
bool rpConnection = false;
std::shared_ptr<Elk::M1Connection> connection;
std::shared_ptr<Elk::M1AsciiAPI> m1api;

void logDataReturnHandler(Elk::LogEntry logEntry) {
	std::cout << "Log Entry " << logEntry.index << ":\n"
		<< "Event: " << logEntry.event << "\n"
		<< "Event Subject Number: " << logEntry.eventSubjectNumber << "\n"
		<< "Area: " << logEntry.area << "\n"
		<< "Time: ";
	switch (logEntry.dayOfWeek) {
	case Elk::Weekday::Sunday:
		std::cout << "Sunday ";
		break;
	case Elk::Weekday::Monday:
		std::cout << "Monday ";
		break;
	case Elk::Weekday::Tuesday:
		std::cout << "Tuesday ";
		break;
	case Elk::Weekday::Wednesday:
		std::cout << "Wednesday ";
		break;
	case Elk::Weekday::Thursday:
		std::cout << "Thursday ";
		break;
	case Elk::Weekday::Friday:
		std::cout << "Friday ";
		break;
	case Elk::Weekday::Saturday:
		std::cout << "Saturday ";
		break;
	default:
		break;
	}
	std::cout<< logEntry.month << "/" << logEntry.day << "/" << logEntry.year
		<< " " << logEntry.hour << ":" << logEntry.minute << "\n";
}

// Map of command names to functions.
// Key can be changed to be more 'cli' like, this is just what my regex spat out.
std::map<std::string, std::function<void()>> commands = {
	{ "activateTask", [] {
		int task;
		std::cout << "Select task: ";
		std::cin >> task;
		m1api->activateTask(task); 
	} },
	{ "armDisarm", [] {
		int partition;
		Elk::ArmMode mode;
		char m;
		std::string userCode;
		std::cout << "Select partition: ";
		std::cin >> partition;
		std::cout << "Select mode (dasingv+-): ";
		std::cin >> m;
		switch (m) {
		case 'd':
			mode = Elk::ARM_DISARMED;
			break;
		case 'a':
			mode = Elk::ARM_AWAY;
			break;
		case 's':
			mode = Elk::ARM_STAY;
			break;
		case 'i':
			mode = Elk::ARM_STAYINSTANT;
			break;
		case 'n':
			mode = Elk::ARM_NIGHT;
			break;
		case 'g':
			mode = Elk::ARM_NIGHTINSTANT;
			break;
		case 'v':
			mode = Elk::ARM_VACATION;
			break;
		case '+':
			mode = Elk::ARM_AWAYNEXT;
			break;
		case '-':
			mode = Elk::ARM_STAYNEXT;
			break;
		default:
			throw std::invalid_argument("Not a valid area mode.");
		}
		
		std::cout << "Please enter user code: ";
		std::cin >> userCode;

		m1api->armDisarm(partition, mode, userCode); 
		std::cout << "\n";
		commands.at("getArmStatus")();
	} },
	{ "disableControlOutput", [] {
		int output;
		std::cout << "Select output: ";
		std::cin >> output;
		m1api->disableControlOutput(output);
	} },
	{ "displayLCDText", [] {
		int area;
		std::string text;
		std::cout << "Select area: ";
		std::cin >> area;
		std::cout << "Select text: ";
		std::cin >> text;
		m1api->displayLCDText(area, Elk::clearMethod::CLEAR_DISPLAY_UNTIL_TIMEOUT, false, 60, text); 
	} },
	{ "enableControlOutput", [] {
		int output;
		uint16_t seconds;
		std::cout << "Select output: ";
		std::cin >> output;
		std::cout << "Select time (seconds): ";
		std::cin >> seconds;
		m1api->enableControlOutput(output, seconds); 
	} },
	{ "executePLCCommand", [] {
		char houseCode;
		int unitCode, functionCode, extendedCode, timeOn;
		std::cout << "Enter House Code[A-P]: ";
		std::cin >> houseCode;
		std::cout << "Enter Unit Code[1-16]: ";
		std::cin >> unitCode;
		std::cout << "Enter Function Code[1-16]: ";
		std::cin >> functionCode;
		std::cout << "Enter Extended Code[0-99]: ";
		std::cin >> extendedCode;
		std::cout << "Enter On Time Seconds[0-9999]: ";
		std::cin >> timeOn;
		m1api->executePLCCommand(houseCode, unitCode, functionCode, extendedCode, timeOn); 
	} },
	{ "getArmStatus", [] {
		int i = 0;
		for (const auto& stat : m1api->getArmStatus()) {
			std::cout << "\"" << m1api->getTextDescription(Elk::TEXT_AreaName, i++) << "\": ";
			switch (stat.mode)
			{
			case Elk::ARM_DISARMED:
				std::cout << "Disarmed, ";
				break;
			case Elk::ARM_AWAY:
				std::cout << "Armed Away, ";
				break;
			case Elk::ARM_STAY:
				std::cout << "Armed Stay, ";
				break;
			case Elk::ARM_STAYINSTANT:
				std::cout << "Armed Stay (instant), ";
				break;
			case Elk::ARM_NIGHT:
				std::cout << "Armed Night, ";
				break;
			case Elk::ARM_NIGHTINSTANT:
				std::cout << "Armed Night (instant), ";
				break;
			case Elk::ARM_VACATION:
				std::cout << "Armed Vacation, ";
				break;
			}

			switch (stat.isReady) {
			case Elk::ARMUPMODE_NOTREADY:
				std::cout << "Not ready to arm ";
				break;
			case Elk::ARMUPMODE_READY:
				std::cout << "Ready to arm ";
				break;
			case Elk::ARMUPMODE_READYFORCE:
				std::cout << "Ready to arm (force) ";
				break;
			case Elk::ARMUPMODE_ARMEDEXITTIMER:
				std::cout << "Armed (exit timer) ";
				break;
			case Elk::ARMUPMODE_ARMED:
				std::cout << "Armed ";
				break;
			case Elk::ARMUPMODE_ARMEDFORCE:
				std::cout << "Armed (force) ";
				break;
			case Elk::ARMUPMODE_ARMEDBYPASS:
				std::cout << "Armed (bypass) ";
				break;
			}
			std::cout << "\n";
		}
	} },
	{ "getAudioData", [] {
		int audioZone;
		std::cout << "Enter Audio Zone:";
		std::cin >> audioZone;
		Elk::AudioData audioData = m1api->getAudioData(audioZone); 
		std::cout << "Zone " << (audioData.source + 1) << ":\n";
		std::cout << "Is on: " << audioData.zoneIsOn << "\n";
		std::cout << "Loudness: " << audioData.loudness << "\n";
		std::cout << "Do Not Disturb: " << audioData.doNotDisturb << "\n";
		std::cout << "Volume: " << audioData.volume << "\n";
		std::cout << "Bass: " << audioData.bass << "\n";
		std::cout << "Treble: " << audioData.treble << "\n";
		std::cout << "Balance: " << audioData.balance << "\n";
		std::cout << "Party Mode: ";
		switch (audioData.partyMode) {
		case Elk::AudioData::PartyMode::PARTYMODE_OFF:
			std::cout << "Off";
			break;
		case Elk::AudioData::PartyMode::PARTYMODE_ON:
			std::cout << "On";
			break;
		case Elk::AudioData::PartyMode::PARTYMODE_MASTER:
			std::cout << "Master";
			break;
		default:
			std::cout << "recieved undefined mode (" << audioData.partyMode << ")";
			break;
		}
		std::cout << "\n";
	} },
	{ "getConfiguredKeypads", [] {
		std::cout << "Configured Keypads: ";
		bool addComma = false;
		for (int keypad : m1api->getConfiguredKeypads()) {
			std::cout << (addComma ? ", " : "") << keypad;
			if (!addComma) addComma = true;
		}
		std::cout << "\n";
	} },
	{ "getConfiguredTempDevices", [] {
		bool addComma = false;
		std::cout << "Configured Temp Devices:\n";
		for (std::pair<int, Elk::TemperatureDevice> p : m1api->getConfiguredTempDevices())
		{
			std::cout << "Device " << p.first << ": ";
			switch (p.second)
			{
			case Elk::TemperatureDevice::TEMPDEVICE_ZONE:
				std::cout << "Zone";
				break;
			case Elk::TemperatureDevice::TEMPDEVICE_KEYPAD:
				std::cout << "Keypad";
				break;
			case Elk::TemperatureDevice::TEMPDEVICE_THERMOSTAT:
				std::cout << "Thermostat";
				break;
			default:
				std::cout << "undefined temperature device (" << p.second << ")";
				break;
			}
			std::cout << "\n";
		}
	} },
	{ "getConfiguredZones", [] {
		bool addComma = false;
		std::cout << "Configured Zones: ";
		for (int z : m1api->getConfiguredZones()) {
			std::cout << (addComma ? ", " : "") << z;
			if (!addComma)
				addComma = true;
		}
		std::cout << "\n";
	} },
	{ "getControlOutputs", [] {
		int i = 0, j = 0;
		for (const auto& out : m1api->getControlOutputs()) {
			std::cout << out << (!(++i % 5) ? " " : "") << (!(++j % 25) ? "\n" : "");
		}
		std::cout << "\n";
	} },
	{ "getCounterValue", [] {
		int counter;
		std::cout << "Select counter index: ";
		std::cin >> counter;
		std::cout << m1api->getCounterValue(counter) << "\n"; 
	} },
	{ "getCustomValue", [] {
		int index;
		std::cout << "Select custom value index: ";
		std::cin >> index;
		std::cout << m1api->getCustomValue(index) << "\n";
	} },
	{ "getCustomValues", [] {
		for (const auto& val : m1api->getCustomValues())
			std::cout << val << "\n";
	} },
	{ "getKeypadAreas", [] {
		const auto& kpa = m1api->getKeypadAreas();
		for (int i : m1api->getConfiguredKeypads()) 
			std::cout << "\"" << m1api->getTextDescription(Elk::TEXT_KeypadName, i) << "\": " << kpa[i] << "\n";
	} },
	{ "getKeypadFkeyStatus", [] {
		int keypad;
		std::cout << "Select keypad: ";
		std::cin >> keypad;
		const auto& kfks = m1api->getKeypadFkeyStatus(keypad); 
		for (int i = 0; i < 6; i++)
		{
			std::cout << "F" << i + 1 << "Illumination : ";
			switch (kfks.illumination[i])
			{
			case Elk::KeypadFkeyStatus::FkeyIllumination::FKEY_OFF:
				std::cout << "OFF";
				break;
			case Elk::KeypadFkeyStatus::FkeyIllumination::FKEY_ON:
				std::cout << "ON";
				break;
			case Elk::KeypadFkeyStatus::FkeyIllumination::FKEY_BLINKING:
				std::cout << "BLINKING";
				break;
			default:
				break;
			}
			std::cout << "\n";
		}
		std::cout << "Code Requred For Bypass: " << (kfks.codeRequiredForBypass ? "Yes" : "No") << "\n";
		for (int i = 0; i < 6; i++) {
			std::cout << "Area " << i + 1 << " Beep Chime Mode: ";
			if (kfks.beepChimeMode[i] == 0)
			{
				std::cout << "OFF";
			}
			else {
				bool comma = false;
				if (kfks.beepChimeMode[i] & Elk::KeypadFkeyStatus::BeepChimeFlags::BCMODE_SINGLE) {
					std::cout << "SINGLE";
					comma = true;
				}
				if (kfks.beepChimeMode[i] & Elk::KeypadFkeyStatus::BeepChimeFlags::BCMODE_CONSTANT) {
					std::cout << (comma ? ", " : "") << "CONSTANT";
					comma = true;
				}
				if (kfks.beepChimeMode[i] & Elk::KeypadFkeyStatus::BeepChimeFlags::BCMODE_SINGLE) {
					std::cout << (comma ? ", " : "") << "CHIME";
				}
			}
			std::cout << "\n";
		}
	} },
	{ "getLightingStatus", [] {
		int device;
		std::cout << "Enter lighting device: ";
		std::cin >> device;
		int status = m1api->getLightingStatus(device); 
		std::cout << "Lighing Device " << device << " Status:" << status << "\n";
	} },
	{ "getLogData", [] {
		int index;
		std::cout << "Enter Log Index[0-510]: ";
		std::cin >> index;
		Elk::LogEntry logEntry = m1api->getLogData(index);
		logDataReturnHandler(logEntry);
	} },
	{ "getLogs", [] {
		std::cout << "getLogs() can take over a minute. Do you wish to continue?[y/n]:";
		char cont;
		std::cin >> cont;
		if (cont == 'y' || cont == 'Y') {
			for (Elk::LogEntry l : m1api->getLogs()) {
				logDataReturnHandler(l);
			}
		}
	} },
	{ "getM1VersionNumber", [] {
		const auto& m1vn = m1api->getM1VersionNumber();
		std::cout << "M1 Version Number: " << m1vn[0] << "."
			<< m1vn[1] << "." << m1vn[2] << "\n";
	} },
	{ "getOmnistat2Data", [] {
		std::cout << "TODO: Write test code \n";
		//m1api->getOmnistat2Data(std::vector<char> request); 
	} },
	{ "getPLCStatus", [] {
		std::cout << "Enter bank[0-4]: ";
		int bank;
		std::cin >> bank;
		std::cout << "Lighting Levels for bank " << bank <<" :";
		int j = 0;
		for (int i : m1api->getPLCStatus(bank)) {
			std::cout << j++ << ": " << i << "\n";
		}
	} },
	{ "getRTCData", [] {
		Elk::RTCData rtc = m1api->getRTCData(); 
		switch (rtc.weekday) {
		case Elk::Sunday:
			std::cout << "Sunday";
			break;
		case Elk::Monday:
			std::cout << "Monday";
			break;
		case Elk::Tuesday:
			std::cout << "Tuesday";
			break;
		case Elk::Wednesday:
			std::cout << "Wednesday";
			break;
		case Elk::Thursday:
			std::cout << "Thursday";
			break;
		case Elk::Friday:
			std::cout << "Friday";
			break;
		case Elk::Saturday:
			std::cout << "Saturday";
			break;
		}
		std::cout << ", " << rtc.year << "/" << rtc.month << "/" << rtc.day << " " << rtc.hours << ":" << rtc.minutes << ":" << rtc.seconds << "\n";
	} },
	{ "getSystemTroubleStatus", [] {
		Elk::SystemTroubleStatus sts = m1api->getSystemTroubleStatus(); 
		std::cout << "System Trouble Status:\n";
		std::cout << "AC Fail: " << sts.ACFail << "\n";
		std::cout << "Box Tamper Zone Number: " << sts.boxTamperZoneNumber << "\n";
		std::cout << "Communication Error: " << sts.communicationError << "\n";
		std::cout << "EEPROM Error: " << sts.EEPROMError << "\n";
		std::cout << "Low Battery Control: " << sts.lowBatteryControl << "\n";
		std::cout << "Transmitter Low Battery Zone Number: " << sts.transmitterLowBatteryZoneNumber << "\n";
		std::cout << "Over Current: " << sts.overCurrent << "\n";
		std::cout << "Telephone Fault: " << sts.telephoneFault << "\n";
		std::cout << "Output 2: " << sts.output2 << "\n";
		std::cout << "Missing Keypad: " << sts.missingKeypad << "\n";
		std::cout << "Zone Expander: " << sts.zoneExpander << "\n";
		std::cout << "Output Expander: " << sts.outputExpander << "\n";
		std::cout << "RP Remote Access: " << sts.RPRemoteAccess << "\n";
		std::cout << "Common Area Not Armed: " << sts.commonAreaNotArmed << "\n";
		std::cout << "Flash Memory Error: " << sts.ACFail << "\n";
		std::cout << "Security Alert Zone Number: " << sts.securityAlertZoneNumber << "\n";
		std::cout << "Serial Port Expander: " << sts.serialPortExpander << "\n";
		std::cout << "Lost Transmitter Zone Number: " << sts.lostTransmitterZoneNumber << "\n";
		std::cout << "GE Smoke Clean Me: " << sts.GESmokeCleanMe << "\n";
		std::cout << "Ethernet: " << sts.ethernet << "\n";
		std::cout << "Display Message Keypad Line 1: " << sts.displayMessageKeypadLine1 << "\n";
		std::cout << "Display Message Keypad Line 2: " << sts.displayMessageKeypadLine2 << "\n";
		std::cout << "Fire Trouble Zone Number: " << sts.fireTroubleZoneNumber << "\n";
	} },
	{ "getTemperature", [] {
		Elk::TemperatureDevice tempDevice;
		while (1) {
			std::cout << "Enter Device Type (zone, keypad, thermostat): ";
			std::string deviceType;
			std::cin >> deviceType;
			if (!deviceType.compare("zone")) {
				tempDevice = Elk::TemperatureDevice::TEMPDEVICE_ZONE;
				break;
			} else if (!deviceType.compare("keypad")) {
				tempDevice = Elk::TemperatureDevice::TEMPDEVICE_KEYPAD;
				break;
			}
			else if (!deviceType.compare("thermostat")) {
				tempDevice = Elk::TemperatureDevice::TEMPDEVICE_THERMOSTAT;
				break;
			}
		}
		int deviceNumber;
		std::cout << "Enter Device Number: ";
		std::cin >> deviceNumber;
		std::cout << "Device " << deviceNumber << " Temp: " << m1api->getTemperature(tempDevice, deviceNumber) << "\n";
	} },
	{ "getTemperatures", [] {
		Elk::TemperatureDevice tempDevice;
		while (1) {
			std::cout << "Enter Device Type (zone, keypad, thermostat): ";
			std::string deviceType;
			std::cin >> deviceType;
			if (!deviceType.compare("zone")) {
				tempDevice = Elk::TemperatureDevice::TEMPDEVICE_ZONE;
				break;
			} else if (!deviceType.compare("keypad")) {
				tempDevice = Elk::TemperatureDevice::TEMPDEVICE_KEYPAD;
				break;
			}
			else if (!deviceType.compare("thermostat")) {
				tempDevice = Elk::TemperatureDevice::TEMPDEVICE_THERMOSTAT;
				break;
			}
		}
		int j = 0;
		for (int i : m1api->getTemperatures(tempDevice)) {
			std::cout << "Device " << j++ << " temp: " << i << "\n";
		}
	} },
	{ "getTextDescription", [] {
		Elk::TextDescriptionType tdt;
		for (;;) {
			std::cout << "Enter Description Type:";
			std::string type;
			std::cin >> type;
			if (!type.compare("zone")) {
				tdt = Elk::TextDescriptionType::TEXT_ZoneName;
				break;
			} else if (!type.compare("area")) {
				tdt = Elk::TextDescriptionType::TEXT_AreaName;
				break;
			} else if (!type.compare("user")) {
				tdt = Elk::TextDescriptionType::TEXT_UserName;
				break;
			} else if (!type.compare("keypad")) {
				tdt = Elk::TextDescriptionType::TEXT_KeypadName;
				break;
			} else if (!type.compare("output")) {
				tdt = Elk::TextDescriptionType::TEXT_OutputName;
				break;
			} else if (!type.compare("task")) {
				tdt = Elk::TextDescriptionType::TEXT_TaskName;
				break;
			} else if (!type.compare("telephone")) {
				tdt = Elk::TextDescriptionType::TEXT_TelephoneName;
				break;
			} else if (!type.compare("light")) {
				tdt = Elk::TextDescriptionType::TEXT_LightName;
				break;
			} else if (!type.compare("alarm")) {
				tdt = Elk::TextDescriptionType::TEXT_AlarmDurationName;
				break;
			} else if (!type.compare("custom")) {
				tdt = Elk::TextDescriptionType::TEXT_CustomSettings;
				break;
			} else if (!type.compare("counter")) {
				tdt = Elk::TextDescriptionType::TEXT_CounterName;
				break;
			} else if (!type.compare("thermostat")) {
				tdt = Elk::TextDescriptionType::TEXT_ThermostatName;
				break;
			} else if (!type.compare("fkey1")) {
				tdt = Elk::TextDescriptionType::TEXT_FKEY1;
				break;
			} else if (!type.compare("fkey2")) {
				tdt = Elk::TextDescriptionType::TEXT_FKEY2;
				break;
			} else if (!type.compare("fkey3")) {
				tdt = Elk::TextDescriptionType::TEXT_FKEY3;
				break;
			} else if (!type.compare("fkey4")) {
				tdt = Elk::TextDescriptionType::TEXT_FKEY4;
				break;
			} else if (!type.compare("fkey5")) {
				tdt = Elk::TextDescriptionType::TEXT_FKEY5;
				break;
			} else if (!type.compare("fkey6")) {
				tdt = Elk::TextDescriptionType::TEXT_FKEY6;
				break;
			} else if (!type.compare("audiozone")) {
				tdt = Elk::TextDescriptionType::TEXT_AudioZoneName;
				break;
			} else if (!type.compare("audiosource")) {
				tdt = Elk::TextDescriptionType::TEXT_AudioSourceName;
				break;
			} else {
				std::cout << "valid inputs: zone, area, user, keypad, output, task, telephone, light, "
					<< "alarm, custom, counter, thermostat, fkey1, fkey2, fkey3, fkey4, fkey5, fkey6, "
					<< "audiozone, auidosource\n";
			}
		}
		std::cout << "Enter Description Index:";
		int index;
		std::cin >> index;
		std::string description = m1api->getTextDescription(tdt, index);
		std::cout << "Text Description: " << description << "\n";
	} },
	{ "getThermostatData", [] {
		std::cout << "Enter Device Index:";
		int index;
		std::cin >> index;
		Elk::ThermostatData tdata = m1api->getThermostatData(index); 
		std::cout << "Thermostat Mode: ";
		switch (tdata.mode) {
		case Elk::ThermostatData::ThermostatMode::OFF:
			std::cout << "Off";
			break;
		case Elk::ThermostatData::ThermostatMode::HEAT:
			std::cout << "Heat";
			break;
		case Elk::ThermostatData::ThermostatMode::COOL:
			std::cout << "Cool";
			break;
		case Elk::ThermostatData::ThermostatMode::AUTO:
			std::cout << "Auto";
			break;
		case Elk::ThermostatData::ThermostatMode::EMERGENCY_HEAT:
			std::cout << "Emergency Heat";
			break;
		default:
			std::cout << "Undefiend Mode";
			break;
		}
		std::cout << "\n";
		std::cout << "Hold Current Temperature: " << tdata.holdCurrentTemperature << "\n";
		std::cout << "Fan On: " << tdata.fanOn << "\n";
		std::cout << "Temperature: " << tdata.temperature << "\n";
		std::cout << "Heat Set Point: " << tdata.heatSetPoint << "\n";
		std::cout << "Cool Set Point: " << tdata.coolSetPoint << "\n";
		std::cout << "Humidity: " << tdata.humidity << "\n";
	} },
	{ "getUserCodeAccess", [] {
		std::cout << "Enter User Code:";
		std::string usercode;
		std::cin >> usercode;
		Elk::UserCodeAccess uca = m1api->getUserCodeAccess(usercode); 
		std::cout << "Code Type: " << uca.codetype << "\n";
		std::cout << "Uses Celcius: " << uca.usesCelcius << "\n";
		std::cout << "Valid Areas: ";
		int addcomma = false;
		for (int i = 0; i < 8; i++) {
			if ((uca.validAreas << i) & 1) {
				std::cout << (addcomma ? ", " : "") << i;
				if (!addcomma)
					addcomma = true;
			}
		}
		std::cout << "\n";
	} },
	{ "getXEPVersionNumber", [] {
		const auto& xvn = m1api->getXEPVersionNumber();
		std::cout << "XEP Version Number: " << xvn[0] << "."
			<< xvn[1] << "." << xvn[2] << "\n";
	} },
	{ "getZoneAlarms", [] {
		int zone = 0;
		for (Elk::SZoneDefinition s : m1api->getZoneAlarms()) {
			std::cout << "Zone " << zone++ << " definition: ";
			switch (s.zd) {
			case Elk::ZoneDefinition::ZONEDEF_DISABLED:
				std::cout << "disabled";
				break;
			case Elk::ZoneDefinition::ZONEDEF_BURGLAR_ENTRY_1:
				std::cout << "burglar entry 1";
				break;
			case Elk::ZoneDefinition::ZONEDEF_BURGLAR_ENTRY_2:
				std::cout << "burglar entry 2";
				break;
			case Elk::ZoneDefinition::ZONEDEF_BURGLAR_PERIMETER_INSTANT:
				std::cout << "burglar perimeter instant";
				break;
			case Elk::ZoneDefinition::ZONEDEF_BURGLAR_INTERIOR:
				std::cout << "burglar interior";
				break;
			case Elk::ZoneDefinition::ZONEDEF_BURGLAR_INTERIOR_FOLLOWER:
				std::cout << "burglar interior follower";
				break;
			case Elk::ZoneDefinition::ZONEDEF_BURGLAR_INTERIOR_NIGHT:
				std::cout << "burglar interior night";
				break;
			case Elk::ZoneDefinition::ZONEDEF_BURGLAR_24_HOUR:
				std::cout << "burglar 24 hour";
				break;
			case Elk::ZoneDefinition::ZONEDEF_BURGLAR_BOX_TAMPER:
				std::cout << "burglar box tamper";
				break;
			case Elk::ZoneDefinition::ZONEDEF_FIRE_ALARM:
				std::cout << "fire alarm";
				break;
			case Elk::ZoneDefinition::ZONEDEF_FIRE_VERIFIED:
				std::cout << "fire verified";
				break;
			case Elk::ZoneDefinition::ZONEDEF_FIRE_SUPERVISORY:
				std::cout << "fire supervisory";
				break;
			case Elk::ZoneDefinition::ZONEDEF_AUX_ALARM_1:
				std::cout << "aux alarm 1";
				break;
			case Elk::ZoneDefinition::ZONEDEF_AUX_ALARM_2:
				std::cout << "aux alarm 2";
				break;
			case Elk::ZoneDefinition::ZONEDEF_KEY_FOB:
				std::cout << "key fob";
				break;
			case Elk::ZoneDefinition::ZONEDEF_NON_ALARM:
				std::cout << "non alarm";
				break;
			case Elk::ZoneDefinition::ZONEDEF_CARBON_MONOXIDE:
				std::cout << "carbon monoxide";
				break;
			case Elk::ZoneDefinition::ZONEDEF_EMERGENCY_ALARM:
				std::cout << "emegency alarm";
				break;
			case Elk::ZoneDefinition::ZONEDEF_FREEZE_ALARM:
				std::cout << "freeze alarm";
				break;
			case Elk::ZoneDefinition::ZONEDEF_GAS_ALARM:
				std::cout << "gas alarm";
				break;
			case Elk::ZoneDefinition::ZONEDEF_HEAT_ALARM:
				std::cout << "heat alarm";
				break;
			case Elk::ZoneDefinition::ZONEDEF_MEDICAL_ALARM:
				std::cout << "medical alarm";
				break;
			case Elk::ZoneDefinition::ZONEDEF_POLICE_ALARM:
				std::cout << "police alarm";
				break;
			case Elk::ZoneDefinition::ZONEDEF_POLICE_NO_INDICATION:
				std::cout << "police no indication";
				break;
			case Elk::ZoneDefinition::ZONEDEF_WATER_ALARM:
				std::cout << "water alarm";
				break;
			case Elk::ZoneDefinition::ZONEDEF_KEY_MOMENTARY_ARMDISARM:
				std::cout << "key momentary armdisarm";
				break;
			case Elk::ZoneDefinition::ZONEDEF_KEY_MOMENTARY_ARM_AWAY:
				std::cout << "key momentary arm away";
				break;
			case Elk::ZoneDefinition::ZONEDEF_KEY_MOMENTARY_ARM_STAY:
				std::cout << "key momentary arm stay";
				break;
			case Elk::ZoneDefinition::ZONEDEF_KEY_MOMENTARY_DISARM:
				std::cout << "key momentary disarm";
				break;
			case Elk::ZoneDefinition::ZONEDEF_KEY_TOGGLE:
				std::cout << "key toggle";
				break;
			case Elk::ZoneDefinition::ZONEDEF_MUTE_AUDIBLES:
				std::cout << "mute audibles";
				break;
			case Elk::ZoneDefinition::ZONEDEF_POWER_SUPERVISORY:
				std::cout << "power supervisory";
				break;
			case Elk::ZoneDefinition::ZONEDEF_TEMPERATURE:
				std::cout << "temperature";
				break;
			case Elk::ZoneDefinition::ZONEDEF_ANALOG_ZONE:
				std::cout << "analog zone";
				break;
			case Elk::ZoneDefinition::ZONEDEF_PHONE_KEY:
				std::cout << "phone key";
				break;
			case Elk::ZoneDefinition::ZONEDEF_INTERCOM_KEY:
				std::cout << "intercom key";
				break;
			default:
				std::cout << "undefined zone definition";
				break;
			}
			std::cout << "\n";
		}
	} },
	{ "getZoneDefinitions", [] {
		std::cout << "Configured Zones: ";
		bool comma = false;
		for (int i : m1api->getConfiguredZones()) {
			std::cout << (comma ? ", " : "") << i;
			comma = true;
		}
		std::cout << "\n";
	} },
	{ "getZonePartitions", [] {
		std::cout << "Zone Partitions: ";
		std::vector<int> partitions = m1api->getZonePartitions();
		bool comma = false;
		for (int i = 0; i < 8; i++) {
			std::cout << "Area " << i << " Zones: ";
			for (int j = 0; j < partitions.size(); j++) {
				if (partitions[j] == i) {
					std::cout << (comma ? ", " : "") << j;
					comma = true;
				}
			}
			std::cout << "\n";
			comma = false;
		}
	} },
	{ "getZoneStatuses", [] {
		int i = 0;
		for (const auto& zs : m1api->getZoneStatuses()) {
			std::cout << "Zone " << ++i << ": ";
			switch (zs.logicalState) {
			case Elk::LZS_BYPASSED:
				std::cout << "Bypassed, ";
				break;
			case Elk::LZS_NORMAL:
				std::cout << "Normal, ";
				break;
			case Elk::LZS_TROUBLE:
				std::cout << "Trouble, ";
				break;
			case Elk::LZS_VIOLATED:
				std::cout << "Violated, ";
				break;
			}
			switch (zs.physicalState) {
			case Elk::PZS_EOL:
				std::cout << "EOL\n";
				break;
			case Elk::PZS_OPEN:
				std::cout << "Open\n";
				break;
			case Elk::PZS_SHORT:
				std::cout << "Short\n";
				break;
			case Elk::PZS_UNCONFIGURED:
				std::cout << "Unconfigured\n";
				break;
			}
		}
	} },
	{ "getZoneVoltage", [] {
		std::cout << "Enter Zone: ";
		int zone;
		std::cin >> zone;
		std::cout << "Zone " << zone << " Voltage: " << m1api->getZoneVoltage(zone) << "\n";
	} },
	{ "pressFunctionKey", [] {
		std::cout << "Enter Keypad: ";
		int keypad;
		std::cin >> keypad;
		Elk::FKEY fkey;
		std::string key;
		for (;;) {
			std::cout << "Enter Key: ";
			std::cin >> key;
			if (!key.compare("1")) {
				fkey = Elk::FKEY::FKEY_1;
				break;
			} else if (!key.compare("2")) {
				fkey = Elk::FKEY::FKEY_2;
				break;
			} else if (!key.compare("3")) {
				fkey = Elk::FKEY::FKEY_3;
				break;
			} else if (!key.compare("4")) {
				fkey = Elk::FKEY::FKEY_4;
				break;
			} else if (!key.compare("5")) {
				fkey = Elk::FKEY::FKEY_5;
				break;
			} else if (!key.compare("6")) {
				fkey = Elk::FKEY::FKEY_6;
				break;
			} else if (!key.compare("star")) {
				fkey = Elk::FKEY::FKEY_STAR;
				break;
			} else if (!key.compare("chime")) {
				fkey = Elk::FKEY::FKEY_CHIME;
				break;
			} else if (!key.compare("none")) {
				fkey = Elk::FKEY::FKEY_NONE;
				break;
			}
			else {
				std::cout << "Enter a value 1-6, start, chime, or none\n";
			}
		}
		int i = 0;
		for (const auto& s : m1api->pressFunctionKey(keypad, fkey)) {
			std::cout << "Area " << i++ << " Chime Mode: ";
			switch (s.cm) {
			case Elk::ChimeMode::CHIMEMODE_OFF:
				std::cout << "Off";
				break;
			case Elk::ChimeMode::CHIMEMODE_CHIMEONLY:
				std::cout << "Chime Only";
				break;
			case Elk::ChimeMode::CHIMEMODE_VOICEONLY:
				std::cout << "Chime Only";
				break;
			case Elk::ChimeMode::CHIMEMODE_BOTH:
				std::cout << "Both";
				break;
			default:
				std::cout << "Undefined Chime Mode";
			}
			std::cout << "\n";
		}
	} },
	{ "requestChangeUserCode", [] {
		std::cout << "Enter User: ";
		int user;
		std::cin >> user;
		std::cout << "Enter Auth Code: ";
		std::string authcode;
		std::cin >> authcode;
		std::cout << "Enter New User Code: ";
		std::string newUserCode;
		std::cin >> newUserCode;
		std::cout << "Enter Area Mask: ";
		std::string areaMask;
		std::cin >> areaMask;
		uint8_t a = stoi(areaMask, 0, 16);
		Elk::UserCodeSuccess ucs = m1api->requestChangeUserCode(user, authcode, newUserCode, a);
		switch (ucs) {
		case Elk::UserCodeSuccess::USERCODE_CHANGE_SUCCESSFUL:
			std::cout << "Usercode Change Successful\n";
			break;
		case Elk::UserCodeSuccess::USERCODE_USER_DUPLICATE:
			std::cout << "Usercode User Duplicate\n";
			break;
		case Elk::UserCodeSuccess::USERCODE_UNAUTHORIZED:
			std::cout << "Usercode Unauthorized\n";
			break;
		default:
			std::cout << "Undefined User Code Success\n";
			break;
		}
	} },
	{ "setAreaBypass", [] {
		std::cout << "Enter Area: ";
		int area;
		std::cin >> area;
		std::string pin;
		std::cout << "Enter Pin Code: ";
		std::cin >> pin;
		bool bypass;
		std::string b;
		for (;;) {
			std::cout << "Bypass Area?[y/n]: ";
			std::cin >> b;
			if (!b.compare("y")) {
				bypass = true;
				break;
			}
			else if (!b.compare("n")) {
				bypass = false;
				break;
			}
			else {
				std::cout << "Enter either \"y\" or \"n\"\n";
			}
		}
		bypass = m1api->setAreaBypass(area, pin, bypass); 
		std::cout << "Area " << area << ((bypass) ? " bypassed" : " unbypassed") << "\n";
	} },
	{ "setCounterValue", [] {
		int counter;
		std::cout << "Enter Counter Number: ";
		std::cin >> counter;
		uint16_t value;
		std::cout << "Enter Counter Value: ";
		std::cin >> value;
		value = m1api->setCounterValue(counter, value); 
		std::cout << "Counter " << counter << " = " << value << "\n";
	} },
	{ "setCustomValue", [] {
		int custom;
		std::cout << "Enter Custom Number: ";
		std::cin >> custom;
		uint16_t value;
		std::cout << "Enter Custom Value: ";
		std::cin >> value;
		m1api->setCustomValue(custom, value); 
		std::cout << "Custom " << custom << " = " << value << "\n";
		//m1api->setCustomValue(int index, uint16_t value); 
	} },
	{ "setLogData", [] {
		int logType, eventType, zoneNumber, area;
		std::cout << "Enter Log Type: ";
		std::cin >> logType;
		std::cout << "Enter Event Type: ";
		std::cin >> eventType;
		std::cout << "Enter Zone Number: ";
		std::cin >> zoneNumber;
		std::cout << "Enter Area: ";
		std::cin >> area;
		m1api->setLogData(logType, eventType, zoneNumber, area); 
	} },
	{ "setPLCState", [] {
		std::cout << "TODO: Write test code";
		//m1api->setPLCState(char houseCode, int unitCode, bool state); 
	} },
	{ "setRTCData", [] {
		std::cout << "TODO: Write test code";
		//m1api->setRTCData(RTCData newData); 
	} },
	{ "setThermostatData", [] {
		std::cout << "TODO: Write test code";
		//m1api->setThermostatData(int index, int value, int element); 
	} },
	{ "speakPhrase", [] {
		std::cout << "TODO: Write test code";
		//m1api->speakPhrase(SirenPhrase phrase); 
	} },
	{ "speakWord", [] {
		std::cout << "TODO: Write test code";
		//m1api->speakWord(SirenWord word); 
	} },
	{ "toggleControlOutput", [] {
		std::cout << "TODO: Write test code";
		//m1api->toggleControlOutput(int output); 
	} },
	{ "togglePLCState", [] {
		std::cout << "TODO: Write test code";
		//m1api->togglePLCState(char houseCode, int unitCode); 
	} },
	{ "zoneBypass", [] {
		std::cout << "TODO: Write test code";
		//m1api->zoneBypass(int zone, std::string pinCode); 
	} },
	{ "quit", [] {
		sigExit = true; 
	} },
	{ "help", [] {
		std::cout << "Available commands: \n";
		for (const auto& cmd : commands)
			std::cout << "\t" << cmd.first << "\n";
		std::cout << "Note that all arguments are 0-indexed.\n";
	} }
};

class CustomArmStatusVectorCallback : public ArmStatusVectorCallback {
public:
	void run(std::vector<Elk::ArmStatus> v) {
		mtx.lock();
		std::cout << "TODO: Custom Arm Status Vector Callback\n";
		mtx.unlock();
	}
};

class CustomDebugOutput : public StringCallback {
public:
	void run(std::string s) {
		mtx.lock();
		std::cout << "DEBUG: " << s << "\n";
		mtx.unlock();
	}
};

class CustomEntryExitTimeDataCallback : public EntryExitTimeDataCallback {
public:
	void run(Elk::EntryExitTimeData eetd) {
		mtx.lock();
		std::cout << "TODO: Custom Entry Exit Time Data Callback\n";
		mtx.unlock();
	}
};

class CustomInvalidUserCodeDataCallback : public InvalidUserCodeDataCallback {
public:
	void run(Elk::InvalidUserCodeData iucd) {
		mtx.lock();
		std::cout << "TODO: Custom Invalid User Code Data Callback\n";
		mtx.unlock();
	}
};

class CustomKeypadFkeyStatusCallback : public KeypadFkeyStatusCallback {
public:
	void run(Elk::KeypadFkeyStatus kfks) {
		mtx.lock();
		std::cout << "TODO: Custom Keypad Fkey Status Callback\n";
		mtx.unlock();
	}
};

class CustomLightingDataCallback : public LightingDataCallback {
public:
	void run(Elk::LightingData ld) {
		mtx.lock();
		std::cout << "TODO: Custom Lighting Data Callback\n";
		mtx.unlock();
	}
};

class CustomLogEntryCallback : public LogEntryCallback {
public:
	void run(Elk::LogEntry) {
		mtx.lock();
		std::cout << "TODO: Custom Log Data Update Callback\n";
		mtx.unlock();
	}
};

class CustomOutputStatusChangeCallback : public BoolVectorCallback {
public:
	void run(std::vector<bool> vb) {
		mtx.lock();
		std::cout << "TODO: Custom Output Status Change Callback\n";
		mtx.unlock();
	}
};

class CustomRPConnectionCallback : public BoolCallback {
public:
	void run(bool b) {
		rpConnection = b;
		mtx.lock();
		std::cout << "RP" << (b ? "Connected" : "Disconnected") << "\n";
		mtx.unlock();
	}
};

class CustomTaskChangeCallback : public IntCallback {
public:
	void run(int i) {
		mtx.lock();
		std::cout << "TODO: Custom Task Change Callback\n";
		mtx.unlock();
	}
};

class CustomValidUserCodeDataCallback : public ValidUserCodeDataCallback {
public:
	void run(Elk::ValidUserCodeData vucd) {
		mtx.lock();
		std::cout << "TODO: Custom Valid User Code Data Callback\n";
		mtx.unlock();
	}
};

class CustomX10DataCallback : public X10DataCallback {
public:
	void run(Elk::X10Data xd) {
		mtx.lock();
		std::cout << "TODO: Custom X10DataCallback Callback\n";
		mtx.unlock();
	}
};

class CustomZoneStateCallback : public ZoneStateCallback {
public:
	void run(Elk::ZoneState zs) {
		mtx.lock();
		std::cout << "TODO: Custom Zone State Callback\n";
		mtx.unlock();
	}
};

int main(int argc, char* argv[])
{
	connection = (std::shared_ptr<Elk::M1Connection>) new Elk::ElkTCP();
	std::string address;
	if (argc < 2) {
		std::cout << "Please enter an address: ";
		std::cin >> address;
	}
	else {
		address = argv[1];
	}
	std::cout << "Connecting...\n";
	if (connection->Connect(address, 2101)) {
		std::cout << "Connected!\n";
		m1api = (std::shared_ptr<Elk::M1AsciiAPI>) new Elk::M1AsciiAPI(connection);

		// uncomment if you want debug output from the API
		//m1api->onDebugOutput = std::shared_ptr<StringCallback>(new CustomDebugOutput());

		m1api->onArmStatusChange = std::shared_ptr<ArmStatusVectorCallback>(new CustomArmStatusVectorCallback());
		m1api->onEntryExitTimerChange = std::shared_ptr<EntryExitTimeDataCallback>(new CustomEntryExitTimeDataCallback());
		m1api->onInvalidUserCodeEntered = std::shared_ptr<InvalidUserCodeDataCallback>(new CustomInvalidUserCodeDataCallback());
		m1api->onKeypadFkeyStatusChange = std::shared_ptr<KeypadFkeyStatusCallback>(new CustomKeypadFkeyStatusCallback());
		m1api->onLightingDataUpdate = std::shared_ptr<LightingDataCallback>(new CustomLightingDataCallback());
		m1api->onLogDataUpdate = std::shared_ptr<LogEntryCallback>(new CustomLogEntryCallback());
		m1api->onOutputStatusChange = std::shared_ptr<BoolVectorCallback>(new CustomOutputStatusChangeCallback());
		m1api->onRPConnection = std::shared_ptr<BoolCallback>(new CustomRPConnectionCallback());
		m1api->onTaskChangeUpdate = std::shared_ptr<IntCallback>(new CustomTaskChangeCallback());
		m1api->onValidUserCodeEntered = std::shared_ptr<ValidUserCodeDataCallback>(new CustomValidUserCodeDataCallback());
		m1api->onX10DataUpdate = std::shared_ptr<X10DataCallback>(new CustomX10DataCallback());
		m1api->onZoneChangeUpdate = std::shared_ptr<ZoneStateCallback>(new CustomZoneStateCallback());

		// Execute
		m1api->run();
		std::cout << "M1 Version: ";
		try {
			for (auto i : m1api->getM1VersionNumber())
				std::cout << i << ".";
		}
		catch (std::exception ex) {
			std::cout << "Error retrieving version number.\n";
			std::cout << ex.what();
		}
		std::cout << "\n";
		std::cout << "Areas: ";
		for (auto i : m1api->getConfiguredAreas()) {
			std::cout << i << " ";
		}
		std::cout << "\n";
		m1api->collectNames(Elk::TEXT_AreaName);
		while (!sigExit) {
			std::string commandIndex;
			if (!rpConnection) {
				mtx.lock();
				std::cout << "------------------------------------------\n";
				std::cout << "Enter a command (or 'help' for help): ";
				std::cin >> commandIndex;
				mtx.unlock();
				try {
					mtx.lock();
					if (!rpConnection)
						commands.at(commandIndex)();
					mtx.unlock();
				}
				catch (std::exception ex) {
					mtx.lock();
					std::cout << "Command failed with the following error: \n";
					std::cout << ex.what() << "\n";
					mtx.unlock();
				}
			}
			else {
				mtx.lock();
				std::cout << "Waiting for RP disconnect...\n";
				mtx.unlock();
				while (rpConnection) {
					std::this_thread::sleep_for(std::chrono::seconds(5));
				}
			}
		}
		m1api->stop();
	}
	else {
		std::cout << "Couldn't connect.\n";
	}
	return 0;
}
