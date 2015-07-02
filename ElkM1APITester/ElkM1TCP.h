/*
	ElkM1TCP.h: Provides C++-only access to an unsecure TCP connection. Not exported with SWIG
	@author Zach Jaggi
*/
#pragma once
#include "ElkM1Connection.h"
#include <WinSock2.h>
#pragma comment(lib, "wsock32.lib")

namespace Elk{

	class ElkTCP : public M1Connection {
	private:
		SOCKET sock;
	public:
		~ElkTCP();
		bool Connect(std::string address, int port);
		void Disconnect();
		void Send(std::vector<char> data);
		std::vector<char> Recieve();
	};
}