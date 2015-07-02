/*
ElkM1Connection.h: Provides the interface(s) for defining a connection to an M1.
@author Zach Jaggi
*/
#pragma once

#include "ElkM1Connection.h"


namespace Elk {
	enum NetworkType {
		NETWORKTYPE_NONE = 0,
		NETWORKTYPE_ETHERNET = 1,
		NETWORKTYPE_CELLULAR = 2
	};

	class C1M1Tunnel : private M1Connection
	{
	private:
		// Method for authenticating an existing connection using a C1M1 authentication packet.
		// TODO: First authentication is with server, which gets us the URL, seconds is with the client, which lets us connect.
		// Example first response: "{ProxyUrl:"www.elklink.com:8892", AuthConnMask:"3", OverDataBudget:True}"
		// Example second response: "{ActualConnMask:1}"
		// C1M1Dispatch contacts the proxy server and gets information
		std::map<std::string, std::string> jsonUglyParse(std::vector<char> input);
		void C1M1Authenticate(std::string username, std::string password, std::string sernum);
		M1Connection* tunnel;
	public:
		ELKM1API C1M1Tunnel(M1Connection* underlying);
		virtual ELKM1API ~C1M1Tunnel();
		ELKM1API NetworkType Authenticate(std::string username, std::string password, std::string sernum);
		ELKM1API bool Connect(std::string location, int port);
		ELKM1API void Disconnect();
		ELKM1API void Send(std::vector<char> data);
		ELKM1API std::vector<char> Recieve();
	};
}
