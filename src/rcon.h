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

class Rcon
{
	public:
		void sendCommand(int port, std::string password, std::string command);
		void connect(int &port, std::string &password);

	protected:

	private:
		struct RConPacket {
			char *cmd;
			unsigned char packetCode;
		};

		Poco::Net::SocketAddress sa;
		Poco::Net::DatagramSocket dgs;
		RConPacket rcon_packet;

		std::clock_t start_time;
		char buffer[1024];

		bool logged_in;
		bool cmd_sent;
		bool cmd_response;
		int size;

		void makePacket(RConPacket rcon, std::string &cmdPacket);
		void sendCommand(std::string &command, std::string &response);
};
