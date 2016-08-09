/*
	ElkM1TCP.cpp: Provides an implementation of M1 TCP Connection 
	@author Zach Jaggi
*/
#include "ElkM1TCP.h"

namespace Elk
{
	ElkTCP::~ElkTCP() {}

	bool ElkTCP::Connect(std::string address, int port) {

#ifdef _WIN32
		WSADATA wsadata;
		int error = WSAStartup(0x0202, &wsadata);

		if (error) return false;

		if (wsadata.wVersion != 0x0202)
		{
			WSACleanup();
			return false;
		}
#endif
		SOCKADDR_IN target;
		target.sin_family = AF_INET;
		target.sin_port = htons(port);
		target.sin_addr.s_addr = inet_addr(address.c_str());

		sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

		if (sock == INVALID_SOCKET) return false;

		return connect(sock, (SOCKADDR*)&target, sizeof(target)) != SOCKET_ERROR;
	}

	void ElkTCP::Disconnect() {
		if (sock) {
#ifdef _WIN32
			int error = closesocket(sock);
#elif __linux__
            int error = close(sock);
#endif
            if (error) {
#ifdef _WIN32
				throw WSAGetLastError();
#elif __linux__
                throw errno;
#endif
			}
		}
#ifdef _WIN32
		WSACleanup();
#endif
	}

	void ElkTCP::Send(std::vector<char> data) {
		int bytes_recieved_or_error = send(sock, &data[0], data.size(), 0);
		if (bytes_recieved_or_error == SOCKET_ERROR) {
#ifdef _WIN32
			throw WSAGetLastError();
#elif __linux__
            throw errno;
#endif
		}
	}

	std::vector<char> ElkTCP::Recieve() {
		std::vector<char> data(4096);
		int bytes_recieved_or_error = recv(sock, &data[0], data.size(), 0);
		if (bytes_recieved_or_error != SOCKET_ERROR) {
			data.resize(bytes_recieved_or_error);
			return data;
		}
		else {
#ifdef _WIN32
			throw WSAGetLastError();
#elif __linux__
            throw errno;
#endif
		}
	}
}
