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

#elif defined(__linux__) || defined(__CYGWIN__)
#define ELKM1API  
#endif

#include <vector>
#include <string>
#include <map>


namespace Elk {
	// Used internally by the ElkM1Monitor class. TODO: If in RP mode, throw exceptions for send/rcv calls
	class M1Connection
	{
	public:
		virtual ELKM1API bool Connect(std::string location, int port) = 0;
		virtual ELKM1API void Disconnect() = 0;
		virtual ELKM1API void Send(std::vector<char> data) = 0;
		virtual ELKM1API std::vector<char> Recieve() = 0; // Recieve data into the buffer. Should block on this call.
	};
}
