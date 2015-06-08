// ElkM1APITester.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <thread>
#include <stdio.h>
#include <map>
#include "ElkM1API.h"
#include "ElkM1AsciiAPI.h"

bool sigExit = false;
Elk::M1Connection *connection;
Elk::M1AsciiAPI *m1api;
// Map of command names to functions.
// Key can be changed to be more 'cli' like, this is just what my regex spat out.
std::map<std::string, std::function<void()>> commands = {
	{ "activateTask", [] {
		int task;
		std::cout << "Select task: ";
		std::cin >> task;
		m1api->activateTask(task); 
	} },
	//{ "armDisarm", [] {m1api->armDisarm(int partition, ArmMode mode, std::string userCode); } },
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
		m1api->displayLCDText(area, Elk::M1API::clearMethod::CLEAR_DISPLAY_UNTIL_TIMEOUT, false, 60, text); 
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
	//{ "getArmStatus", [] {m1api->getArmStatus(); } },
	//{ "getAudioData", [] {m1api->getAudioData(int audioZone); } },
	{ "getControlOutputs", [] {
		for (auto out : m1api->getControlOutputs()) {
			std::cout << " ";
			for (int i = 0; i < 5; i++) {
				for (int j = 0; j < 5; j++) {
					std::cout << out;
				}
				std::cout << " ";
			}
			std::cout << "\n";
		}
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
		for (auto val : m1api->getCustomValues())
			std::cout << val << "\n";
	} },
	{ "getKeypadAreas", [] {
		for (auto keypad : m1api->getKeypadAreas())
			std::cout << keypad << " ";
		std::cout << "\n";
	} },
	//{ "getKeypadFkeyStatus", [] {m1api->getKeypadFkeyStatus(int keypad); } },
	//{ "getLightingStatus", [] {m1api->getLightingStatus(int device); } },
	//{ "getLogData", [] {m1api->getLogData(int index); } },
	//{ "getLogs", [] {m1api->getLogs(); } },
	//{ "getM1VersionNumber", [] {m1api->getM1VersionNumber(); } },
	//{ "getOmnistat2Data", [] {m1api->getOmnistat2Data(std::vector<char> request); } },
	//{ "getPLCStatus", [] {m1api->getPLCStatus(int bank); } },
	//{ "getRTCData", [] {m1api->getRTCData(); } },
	//{ "getSystemTroubleStatus", [] {m1api->getSystemTroubleStatus(); } },
	//{ "getTemperature", [] {m1api->getTemperature(TemperatureDevice type, int device); } },
	//{ "getTemperatures", [] {m1api->getTemperatures(TemperatureDevice type); } },
	//{ "getTextDescription", [] {m1api->getTextDescription(TextDescriptionType type, int index); } },
	//{ "getThermostatData", [] {m1api->getThermostatData(int index); } },
	//{ "getUserCodeAccess", [] {m1api->getUserCodeAccess(std::string userCode); } },
	//{ "getZoneAlarms", [] {m1api->getZoneAlarms(); } },
	//{ "getZoneDefinitions", [] {m1api->getZoneDefinitions(); } },
	//{ "getZonePartitions", [] {m1api->getZonePartitions(); } },
	//{ "getZoneStatuses", [] {m1api->getZoneStatuses(); } },
	{ "getZoneVoltage", [] {
		int zone;
		std::cout << "Select zone: ";
		std::cin >> zone;
		m1api->getZoneVoltage(zone); 
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
		for (auto cmd : commands)
			std::cout << "\t" << cmd.first << "\n";
		std::cout << "Note that all arguments are 0-indexed.\n";
	} }
};


int main(int argc, char* argv[])
{
	connection = new Elk::ElkTCP();
	std::string address;
	if (argc < 2) {
		std::cout << "Please enter an address: ";
		std::cin >> address;
	}
	else {
		address = argv[1];
	}
	std::cout << "Connecting...\n";
	if (connection->Connect(address)) {
		std::cout << "Connected!\n";
		m1api = new Elk::M1AsciiAPI(connection);
		m1api->run();
		std::cout << "M1 Version: ";
		for (auto i : m1api->getM1VersionNumber())
			std::cout << i << ".";
		std::cout << "\n";
		while (!sigExit) {
			std::string commandIndex;
			std::cout << "------------------------------------------\n";
			std::cout << "Enter a command (or 'help' for help): ";
			std::cin >> commandIndex;
			try {
				commands.at(commandIndex)();
			}
			catch (std::exception ex) {
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