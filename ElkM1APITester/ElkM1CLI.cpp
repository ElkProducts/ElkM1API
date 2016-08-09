// ElkM1APITester.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <thread>
#include <stdio.h>
#include <memory>
#include <map>
#include "ElkM1API.h"
#include "ElkM1AsciiAPI.h"
#include "ElkM1TCP.h"

bool sigExit = false;
std::shared_ptr<Elk::M1Connection> connection;
std::shared_ptr<Elk::M1AsciiAPI> m1api;
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
	//{ "executePLCCommand", [] {m1api->executePLCCommand(char houseCode, int unitCode, int functionCode, int extendedCode, int timeOn); } },
	{ "getArmStatus", [] {
		int i = 0;
		for (const auto& stat : m1api->getArmStatus()) {
			std::cout << "\"" << m1api->getTextDescription(Elk::TEXT_AreaName, i) << "\": ";
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
	//{ "getAudioData", [] {m1api->getAudioData(int audioZone); } },
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
		for (int i = 0; i < 16; i++)
			std::cout << "\"" << m1api->getTextDescription(Elk::TEXT_KeypadName, i) << "\": " << kpa[i] << "\n";
	} },
	//{ "getKeypadFkeyStatus", [] {m1api->getKeypadFkeyStatus(int keypad); } },
	//{ "getLightingStatus", [] {m1api->getLightingStatus(int device); } },
	//{ "getLogData", [] {m1api->getLogData(int index); } },
	//{ "getLogs", [] {m1api->getLogs(); } },
	//{ "getOmnistat2Data", [] {m1api->getOmnistat2Data(std::vector<char> request); } },
	//{ "getPLCStatus", [] {m1api->getPLCStatus(int bank); } },
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
	//{ "getSystemTroubleStatus", [] {m1api->getSystemTroubleStatus(); } },
	//{ "getTemperature", [] {m1api->getTemperature(TemperatureDevice type, int device); } },
	{ "getTemperatures", [] {
		//m1api->forEachConfiguredTempDevice([](Elk::TemperatureDevice dev, int index) {
		//	switch (dev) {
		//	case Elk::TEMPDEVICE_THERMOSTAT:
		//		try {
		//			std::cout << "\"" << m1api->getTextDescription(Elk::TEXT_ThermostatName, dev) << "\": " << m1api->getTemperature(dev, index) << "\n";
		//		}
		//		catch (...) {
		//			std::cout << "Thermostat " << index << ": " << m1api->getTemperature(dev, index) << "\n";
		//		}
		//		break;
		//	case Elk::TEMPDEVICE_KEYPAD:
		//		try {
		//			std::cout << "\"" << m1api->getTextDescription(Elk::TEXT_KeypadName, dev) << "\": " << m1api->getTemperature(dev, index) << "\n";
		//		}
		//		catch (...) {
		//			std::cout << "Keypad " << index << ": " << m1api->getTemperature(dev, index) << "\n";
		//		}
		//		break;
		//	case Elk::TEMPDEVICE_ZONE:
		//		std::cout << "Temp Zone " << index << ": " << m1api->getTemperature(dev, index) << "\n";
		//		break;
		//	}
		//});
	} },
	//{ "getThermostatData", [] {m1api->getThermostatData(int index); } },
	//{ "getUserCodeAccess", [] {m1api->getUserCodeAccess(std::string userCode); } },
	{ "getZoneAlarms", [] {
		const auto& zdfs = m1api->getZoneAlarms();
		//m1api->forEachConfiguredZone([&zdfs](int index) {
		//	std::cout << "\"" << m1api->getTextDescription(Elk::TEXT_ZoneName, index) << "\": ";
		//	switch (zdfs[index].zd) {
		//	case Elk::ZONEDEF_DISABLED:
		//		std::cout << "Disabled"; break;
		//	case Elk::ZONEDEF_BURGLAR_ENTRY_1:
		//		std::cout << "Burglar entry 1"; break;
		//	case Elk::ZONEDEF_BURGLAR_ENTRY_2:
		//		std::cout << "Burglar entry 2"; break;
		//	case Elk::ZONEDEF_BURGLAR_PERIMETER_INSTANT:
		//		std::cout << "Burglar perimeter instant"; break;
		//	case Elk::ZONEDEF_BURGLAR_INTERIOR:
		//		std::cout << "Burglar interior"; break;
		//	case Elk::ZONEDEF_BURGLAR_INTERIOR_FOLLOWER:
		//		std::cout << "Burglar follower"; break;
		//	case Elk::ZONEDEF_BURGLAR_INTERIOR_NIGHT:
		//		std::cout << "Burglar interior night"; break;
		//	case Elk::ZONEDEF_BURGLAR_INTERIOR_NIGHT_DELAY:
		//		std::cout << "Burglar interior night delay"; break;
		//	case Elk::ZONEDEF_BURGLAR_24_HOUR:
		//		std::cout << "Burglar 24 hour"; break;
		//	case Elk::ZONEDEF_BURGLAR_BOX_TAMPER:
		//		std::cout << "Burglar box tamper"; break;
		//	case Elk::ZONEDEF_FIRE_ALARM:
		//		std::cout << "Fire alarm"; break;
		//	case Elk::ZONEDEF_FIRE_VERIFIED:
		//		std::cout << "Fire verified"; break;
		//	case Elk::ZONEDEF_FIRE_SUPERVISORY:
		//		std::cout << "Fire supervisory"; break;
		//	case Elk::ZONEDEF_AUX_ALARM_1:
		//		std::cout << "Aux alarm 1"; break;
		//	case Elk::ZONEDEF_AUX_ALARM_2:
		//		std::cout << "Aux alarm 2"; break;
		//	case Elk::ZONEDEF_KEY_FOB:
		//		std::cout << "Keyfob"; break;
		//	case Elk::ZONEDEF_NON_ALARM:
		//		std::cout << "Non alarm"; break;
		//	case Elk::ZONEDEF_CARBON_MONOXIDE:
		//		std::cout << "Carbon monoxide"; break;
		//	case Elk::ZONEDEF_EMERGENCY_ALARM:
		//		std::cout << "Emergency alarm"; break;
		//	case Elk::ZONEDEF_FREEZE_ALARM:
		//		std::cout << "Freeze alarm"; break;
		//	case Elk::ZONEDEF_GAS_ALARM:
		//		std::cout << "Gas alarm"; break;
		//	case Elk::ZONEDEF_HEAT_ALARM:
		//		std::cout << "Heat alarm"; break;
		//	case Elk::ZONEDEF_MEDICAL_ALARM:
		//		std::cout << "Medical alarm"; break;
		//	case Elk::ZONEDEF_POLICE_ALARM:
		//		std::cout << "Police alarm"; break;
		//	case Elk::ZONEDEF_POLICE_NO_INDICATION:
		//		std::cout << "Police no indication alarm"; break;
		//	case Elk::ZONEDEF_WATER_ALARM:
		//		std::cout << "Water alarm"; break;
		//	case Elk::ZONEDEF_KEY_MOMENTARY_ARMDISARM:
		//		std::cout << "Momentary arm/disarm"; break;
		//	case Elk::ZONEDEF_KEY_MOMENTARY_ARM_AWAY:
		//		std::cout << "Momentary arm away"; break;
		//	case Elk::ZONEDEF_KEY_MOMENTARY_ARM_STAY:
		//		std::cout << "Momentary arm stay"; break;
		//	case Elk::ZONEDEF_KEY_MOMENTARY_DISARM:
		//		std::cout << "Momentary disarm"; break;
		//	case Elk::ZONEDEF_KEY_TOGGLE:
		//		std::cout << "Key toggle"; break;
		//	case Elk::ZONEDEF_MUTE_AUDIBLES:
		//		std::cout << "Mute audibles"; break;
		//	case Elk::ZONEDEF_POWER_SUPERVISORY:
		//		std::cout << "Power supervisory"; break;
		//	case Elk::ZONEDEF_TEMPERATURE:
		//		std::cout << "Temperature"; break;
		//	case Elk::ZONEDEF_ANALOG_ZONE:
		//		std::cout << "Analog zone"; break;
		//	case Elk::ZONEDEF_PHONE_KEY:
		//		std::cout << "Phone key"; break;
		//	case Elk::ZONEDEF_INTERCOM_KEY:
		//		std::cout << "Intercom key"; break;
		//	}
		//	std::cout << "\n";
		//});
	} },
	{ "getZoneDefinitions", [] {
		const auto& zdfs = m1api->getZoneDefinitions(); 
		//m1api->forEachConfiguredZone([&zdfs](int index) {
		//	std::cout << "\"" << m1api->getTextDescription(Elk::TEXT_ZoneName, index) << "\": ";
		//	switch (zdfs[index].zd) {
		//	case Elk::ZONEDEF_DISABLED:
		//		std::cout << "Disabled"; break;
		//	case Elk::ZONEDEF_BURGLAR_ENTRY_1:
		//		std::cout << "Burglar entry 1"; break;
		//	case Elk::ZONEDEF_BURGLAR_ENTRY_2:
		//		std::cout << "Burglar entry 2"; break;
		//	case Elk::ZONEDEF_BURGLAR_PERIMETER_INSTANT:
		//		std::cout << "Burglar perimeter instant"; break;
		//	case Elk::ZONEDEF_BURGLAR_INTERIOR:
		//		std::cout << "Burglar interior"; break;
		//	case Elk::ZONEDEF_BURGLAR_INTERIOR_FOLLOWER:
		//		std::cout << "Burglar follower"; break;
		//	case Elk::ZONEDEF_BURGLAR_INTERIOR_NIGHT:
		//		std::cout << "Burglar interior night"; break;
		//	case Elk::ZONEDEF_BURGLAR_INTERIOR_NIGHT_DELAY:
		//		std::cout << "Burglar interior night delay"; break;
		//	case Elk::ZONEDEF_BURGLAR_24_HOUR:
		//		std::cout << "Burglar 24 hour"; break;
		//	case Elk::ZONEDEF_BURGLAR_BOX_TAMPER:
		//		std::cout << "Burglar box tamper"; break;
		//	case Elk::ZONEDEF_FIRE_ALARM:
		//		std::cout << "Fire alarm"; break;
		//	case Elk::ZONEDEF_FIRE_VERIFIED:
		//		std::cout << "Fire verified"; break;
		//	case Elk::ZONEDEF_FIRE_SUPERVISORY:
		//		std::cout << "Fire supervisory"; break;
		//	case Elk::ZONEDEF_AUX_ALARM_1:
		//		std::cout << "Aux alarm 1"; break;
		//	case Elk::ZONEDEF_AUX_ALARM_2:
		//		std::cout << "Aux alarm 2"; break;
		//	case Elk::ZONEDEF_KEY_FOB:
		//		std::cout << "Keyfob"; break;
		//	case Elk::ZONEDEF_NON_ALARM:
		//		std::cout << "Non alarm"; break;
		//	case Elk::ZONEDEF_CARBON_MONOXIDE:
		//		std::cout << "Carbon monoxide"; break;
		//	case Elk::ZONEDEF_EMERGENCY_ALARM:
		//		std::cout << "Emergency alarm"; break;
		//	case Elk::ZONEDEF_FREEZE_ALARM:
		//		std::cout << "Freeze alarm"; break;
		//	case Elk::ZONEDEF_GAS_ALARM:
		//		std::cout << "Gas alarm"; break;
		//	case Elk::ZONEDEF_HEAT_ALARM:
		//		std::cout << "Heat alarm"; break;
		//	case Elk::ZONEDEF_MEDICAL_ALARM:
		//		std::cout << "Medical alarm"; break;
		//	case Elk::ZONEDEF_POLICE_ALARM:
		//		std::cout << "Police alarm"; break;
		//	case Elk::ZONEDEF_POLICE_NO_INDICATION:
		//		std::cout << "Police no indication alarm"; break;
		//	case Elk::ZONEDEF_WATER_ALARM:
		//		std::cout << "Water alarm"; break;
		//	case Elk::ZONEDEF_KEY_MOMENTARY_ARMDISARM:
		//		std::cout << "Momentary arm/disarm"; break;
		//	case Elk::ZONEDEF_KEY_MOMENTARY_ARM_AWAY:
		//		std::cout << "Momentary arm away"; break;
		//	case Elk::ZONEDEF_KEY_MOMENTARY_ARM_STAY:
		//		std::cout << "Momentary arm stay"; break;
		//	case Elk::ZONEDEF_KEY_MOMENTARY_DISARM:
		//		std::cout << "Momentary disarm"; break;
		//	case Elk::ZONEDEF_KEY_TOGGLE:
		//		std::cout << "Key toggle"; break;
		//	case Elk::ZONEDEF_MUTE_AUDIBLES:
		//		std::cout << "Mute audibles"; break;
		//	case Elk::ZONEDEF_POWER_SUPERVISORY:
		//		std::cout << "Power supervisory"; break;
		//	case Elk::ZONEDEF_TEMPERATURE:
		//		std::cout << "Temperature"; break;
		//	case Elk::ZONEDEF_ANALOG_ZONE:
		//		std::cout << "Analog zone"; break;
		//	case Elk::ZONEDEF_PHONE_KEY:
		//		std::cout << "Phone key"; break;
		//	case Elk::ZONEDEF_INTERCOM_KEY:
		//		std::cout << "Intercom key"; break;
		//	}
		//	std::cout << "\n";
		//});
	} },
	{ "getZonePartitions", [] {
		const auto& parts = m1api->getZonePartitions();
		//m1api->forEachConfiguredZone([parts](int index) {
		//	std::cout << "\"" << m1api->getTextDescription(Elk::TEXT_ZoneName, index) << "\" partition: " << parts[index] << "\n";
		//});
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
	{ "getZoneVoltages", [] {
		//m1api->forEachConfiguredZone([](int index) {
		//	std::cout << "\"" << m1api->getTextDescription(Elk::TEXT_ZoneName, index) << "\" voltage: " << m1api->getZoneVoltage(index) << "\n";
		//});
	} },
	//{ "pressFunctionKey", [] {m1api->pressFunctionKey(int keypad, FKEY key); } },
	//{ "requestChangeUserCode", [] {m1api->requestChangeUserCode(int user, std::string authCode, std::string newUserCode, uint8_t areaMask); } },
	//{ "setAreaBypass", [] {m1api->setAreaBypass(int area, std::string pinCode, bool bypassed); } },
	//{ "setCounterValue", [] {m1api->setCounterValue(int counter, uint16_t value); } },
	//{ "setCustomValue", [] {m1api->setCustomValue(int index, uint16_t value); } },
	//{ "setLogData", [] {m1api->setLogData(int logType, int eventType, int zoneNumber, int area); } },
	//{ "setPLCState", [] {m1api->setPLCState(char houseCode, int unitCode, bool state); } },
	//{ "setRTCData", [] {m1api->setRTCData(RTCData newData); } },
	//{ "setThermostatData", [] {m1api->setThermostatData(int index, int value, int element); } },
	//{ "speakPhrase", [] {m1api->speakPhrase(SirenPhrase phrase); } },
	//{ "speakWord", [] {m1api->speakWord(SirenWord word); } },
	//{ "toggleControlOutput", [] {m1api->toggleControlOutput(int output); } },
	//{ "togglePLCState", [] {m1api->togglePLCState(char houseCode, int unitCode); } },
	//{ "zoneBypass", [] {m1api->zoneBypass(int zone, std::string pinCode); } }
	{ "quit", [] {sigExit = true; } },
	{ "help", [] {
		std::cout << "Available commands: \n";
		for (const auto& cmd : commands)
			std::cout << "\t" << cmd.first << "\n";
		std::cout << "Note that all arguments are 0-indexed.\n";
	} }
};

class CallbackCheck : public ArmStatusVectorCallback {
public:
	void run(std::vector<Elk::ArmStatus> v) {
		std::cout << "Meow!\n";
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

		// Give it callbacks
		//m1api->onRPConnection = [] (bool connected){
		//	std::cout << "-- Warning, RP " << (connected ? "connected" : "disconnected") << ". --\n";
		//};

		m1api->onArmStatusChange = std::shared_ptr<ArmStatusVectorCallback>(new CallbackCheck());



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
			std::cout << "------------------------------------------\n";
			std::cout << "Enter a command (or 'help' for help): ";
			std::cin >> commandIndex;
			try {
				commands.at(commandIndex)();
			}
			catch (std::exception ex) {
				std::cout << "Command failed with the following error: \n";
				std::cout << ex.what() << "\n";
			}
		}
		m1api->stop();
	}
	else {
		std::cout << "Couldn't connect.\n";
	}
	return 0;
}
