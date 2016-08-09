/*
	ElkM1TCP.h: Provides C++-only access to an unsecure TCP connection. Not exported with SWIG
	@author Zach Jaggi
*/
#pragma once
#include "ElkM1Connection.h"

#ifdef _WIN32
#include <WinSock2.h>
#pragma comment(lib, "wsock32.lib")
#elif __linux__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define SOCKET int
#define SOCKADDR_IN sockaddr_in
#define SOCKADDR sockaddr
#define SOCKET_ERROR -1 
#define INVALID_SOCKET -1
#endif

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
