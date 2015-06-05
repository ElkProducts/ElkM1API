/*
	ElkM1Connection.h: Provides the interface(s) for defining a connection to an M1.
	@author Zach Jaggi
*/
#pragma once

#ifdef _WIN32

#ifdef ELKM1API_EXPORTS
#define ELKM1API __declspec(dllexport)
#else
#define ELKM1API __declspec(dllimport)
#endif

#include <WinSock2.h>

#elif __linux__
#define ELKM1API  
#endif

#include <vector>


namespace Elk {
	// Used internally by the ElkM1Monitor class. TODO: If in RP mode, throw exceptions for send/rcv calls
	class M1Connection
	{
	public:
		virtual ELKM1API bool Connect(std::string location) = 0;
		virtual ELKM1API void Disconnect() = 0;
		virtual ELKM1API void Send(std::vector<char> data) = 0;
		virtual ELKM1API std::vector<char> Recieve() = 0; // Recieve data into the buffer. Should block on this call.
	};

	class ElkTCP : public M1Connection {
	private:
		SOCKET sock;
	public:
		ELKM1API bool Connect(std::string location);
		ELKM1API bool Connect(std::string address, int port);
		ELKM1API void Disconnect();
		ELKM1API void Send(std::vector<char> data);
		ELKM1API std::vector<char> Recieve();
	};

	// TODO: Implement secure connection via SSL

	// TODO: Implement secure proxied connection via SSL and C1M1 Proxy
	
}