// ElkM1APITester.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <thread>
#include <stdio.h>
#include "ElkM1API.h"
#include "ElkM1AsciiAPI.h"

bool sigExit = false;
Elk::M1Connection *connection;
Elk::M1AsciiAPI *m1api;

int main(int argc, char* argv[])
{
	connection = new Elk::ElkTCP();
	std::string address;
	std::string text;
	int area, zone;
	if (argc < 2) {
		std::cout << "Please enter an address: ";
		std::cin >> address;
	}
	else {
		address = argv[1];
	}
	std::cout << "Connecting...\n";
	if (connection->Connect(address)) {
		std::cout << "\rConnected!\n";
		m1api = new Elk::M1AsciiAPI(connection);
		m1api->run();
		std::cout << "M1 Version: ";
		for (auto i : m1api->getM1VersionNumber())
			std::cout << i << ".";
		std::cout << "\n";
		while (!sigExit) {
			std::string command;
			std::cout << "------------------------------------";
			std::cout << "Enter a command (or 'h' for help): ";
			std::cin >> command;
			switch (command[0]) {
			case 'h':
				std::cout << "\th:\tShow this help\n";
				std::cout << "\tq:\tQuit\n";
				std::cout << "\td:\tDisplay text on LCD screens\n";
				std::cout << "\tv:\tDisplay zone voltage\n";
				break;
			case 'q':
				sigExit = true;
				break;
			case 'd':
				std::cout << "Select area: ";
				std::cin >> area;
				std::cout << "Select text: ";
				std::cin >> text;
				m1api->displayLCDText(area - 1, Elk::M1API::CLEAR_DISPLAY_UNTIL_TIMEOUT, false, 30, text);
				break;
			case 'v':
				std::cout << "Select zone (0 for all): ";
				std::cin >> zone;
				if (zone) {
					std::cout << "Zone " << zone << " voltage: " << m1api->getZoneVoltage(zone - 1) << "\n";
				}
				else {
					m1api->forEachConfiguredZone([](int zone) {
						std::cout << "Zone " << (zone + 1) << " voltage: " << m1api->getZoneVoltage(zone) << "\n";
					});
				}
				break;
			default:
				std::cout << "Command not recognised.\n";
			}
		}
		m1api->stop();
	}
	else {
		std::cout << "Couldn't connect.\n";
	}
	return 0;
}