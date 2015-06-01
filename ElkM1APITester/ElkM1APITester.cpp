// ElkM1APITester.cpp : Defines the entry point for the console application.
//

#include <iostream>
#include <thread>
#include <stdio.h>
#include "ElkM1API.h"
#include "ElkM1AsciiAPI.h"

void test_M1TCP() {
	Elk::M1Connection *inst = new Elk::ElkTCP();
	if (inst->Connect("127.0.0.1")) {
		std::cout << "Worked!\n" << "Sending boots...\n";
		std::string s = std::string("Boots\n");
		inst->Send(std::vector<char>(s.begin(), s.end()));
		std::cout << "Recieving whatever...\n" << std::string(&inst->Recieve()[0]);
	}
	else {
		std::cout << "Didn't connect.\n";
		inst->Disconnect();
		delete inst;
		inst = nullptr;
	}
}

void test_M1Monitor() {
	Elk::M1Connection *inst = new Elk::ElkTCP();
	if (inst->Connect("192.168.101.104")) { // Luke
		std::cout << "Connected!\n";
		Elk::M1AsciiAPI m1api = Elk::M1AsciiAPI(inst);
		m1api.run();
		m1api.displayLCDText(0, Elk::M1API::CLEAR_DISPLAY_UNTIL_TIMEOUT, false, 30, "Meowtesting");
		m1api.forEachConfiguredZone([&m1api](int zone) {
			std::cout << "Zone " << (zone + 1) << " name: " << m1api.getTextDescription(Elk::M1API::TDT_ZoneName, zone) << "\n";
		});
		system("pause");
		m1api.stop();
	}
	else {
		std::cout << "Couldn't connect.\n";
		system("pause");
	}
}

int main(int argc, char* argv[])
{
	test_M1Monitor();
	return 0;
}