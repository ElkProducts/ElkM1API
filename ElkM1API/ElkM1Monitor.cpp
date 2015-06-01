/*
	ElkM1Monitor.cpp: Provides an implementation of the M1Monitor cached M1 connection service class.
	@author Zach Jaggi
*/

#include "ElkM1Monitor.h"

namespace Elk
{
	M1Monitor::M1Monitor(M1Connection* conn) {
		if (conn == nullptr)
		{
			throw std::invalid_argument("Connection must exist!");
		}
		connection = conn;
	}

	void M1Monitor::run() {
		// Start the thread...
		executionThread = std::thread(&M1Monitor::_start, this);
	}

	void M1Monitor::stop() {
		sigStop = true;
		try{
			connection->Disconnect();
		}
		catch (...) {
			// Must be already closed
		}
		// Tell the execution thread to rejoin, if we aren't it
		if (std::this_thread::get_id() != executionThread.get_id())
			executionThread.join();
	}

	void M1Monitor::_start() {
		while (!sigStop) {
			// Block on ElkConnection::Recieve, Add new message to message buffer
			std::vector<char> newData;
			try{
				newData = connection->Recieve();
			}
			// TODO: Cleaner implementation
			catch (...) {
				stop();
				return;
			}
			buffer.insert(buffer.end(), newData.begin(), newData.end());
			// TODO: Replace below with 'cutmessage' function
			boolean noNewMessages = false;
			while (buffer.size() > 0 && !noNewMessages) {
				std::vector<char> newMessage = cutMessage(buffer);
				noNewMessages = (newMessage.size() == 0);
				handleMessage(newMessage);
			}
		}
	}

	M1Monitor::~M1Monitor() {
		delete connection;
	}


}