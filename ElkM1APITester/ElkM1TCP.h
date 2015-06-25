/*
	ElkM1TCP.h: Provides C++-only access to an unsecure TCP connection. Not exported with SWIG
	@author Zach Jaggi
*/
#pragma once
#include "ElkM1Connection.h"

namespace Elk{

	class ElkTCP : public M1Connection {
	private:
		SOCKET sock;
	public:
		ELKM1API bool Connect(std::string address, int port);
		ELKM1API void Disconnect();
		ELKM1API void Send(std::vector<char> data);
		ELKM1API std::vector<char> Recieve();
	};
}