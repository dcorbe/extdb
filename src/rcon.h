/*
Copyright (C) 2014 Declan Ireland <http://github.com/torndeco/extDB>
Copyright (C) 2012 Prithu "bladez" Parker <https://github.com/bladez-/bercon>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

 * Change Log
 * Changed Code to use Poco Net Library 
*/


#pragma once

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/NetException.h>

//#include <boost/thread/thread.hpp>

class Rcon
{
	public:
		void sendCommand(std::string command);
		void init(int port, std::string password);

	private:
		struct RconPacket {
			char cmd_char_workaround;
			char *cmd;
			unsigned char packetCode;
		};

		struct RconLogin {
			int port;
			char *password;
		};


		Poco::Net::SocketAddress sa;
		Poco::Net::DatagramSocket dgs;

		RconLogin rcon_login;
		RconPacket rcon_packet;

		std::clock_t start_time;
		char buffer[1024];  //TODO Change so not hardcoded limit
		int buffer_size;

		bool logged_in;
		bool cmd_sent;
		bool cmd_response;
		int size;
		
		//boost::mutex mutex_rcon_global; // TODO:: Look @ changing Rcon Code to avoid this

		void connect();
		void sendCommand(std::string &command, std::string &response);
		void makePacket(RconPacket rcon, std::string &cmdPacket);
		void extractData(int pos, std::string data, std::string &result);
};
