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


// DatagramSocket send example
#include <Poco/Checksum.h>

#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Timestamp.h>
#include <Poco/DateTimeFormatter.h>

#include <sstream>
#include <iomanip>
#include <iostream>

struct RConPacket {
	char *cmd;
	unsigned char packetCode;
};

void makePacket(RConPacket rcon, std::string &cmdPacket)
{
	Poco::Checksum checksum_crc32;

	std::ostringstream cmdStream;
	cmdStream.put(0xFF);
	cmdStream.put(rcon.packetCode);

	if (rcon.packetCode == 0x01)
	{
		cmdStream.put(0x00);
	}

	cmdStream << rcon.cmd;
	std::string cmd = cmdStream.str();

	checksum_crc32.update(cmd);
	long int crcVal = checksum_crc32.checksum();

	std::stringstream hexStream;
	hexStream << std::setfill('0') << std::setw(sizeof(int)*2) << std::hex << crcVal;
	std::string crcAsHex = hexStream.str();

	unsigned char reversedCrc[4];
	unsigned int x;

	std::stringstream converterStream;

	for (int i = 0; i < 4; i++)
	{
		converterStream << std::hex << crcAsHex.substr(6-(2*i),2).c_str();
		converterStream >> x;
		converterStream.clear();
		reversedCrc[i] = x;
	}

	std::stringstream cmdPacketStream;

	cmdPacketStream.put(0x42);
	cmdPacketStream.put(0x45);
	cmdPacketStream.put(reversedCrc[0]);
	cmdPacketStream.put(reversedCrc[1]);
	cmdPacketStream.put(reversedCrc[2]);
	cmdPacketStream.put(reversedCrc[3]);
	cmdPacketStream << cmd;
	cmdPacket = cmdPacketStream.str();
}


int main(int argc, char* argv[])
{
	Poco::Net::SocketAddress sa("localhost", 2302);
	Poco::Net::DatagramSocket dgs;
	dgs.connect(sa);

	RConPacket rcon_packet;
	char *rconPass;
	rconPass = argv[1];
	std::cout << "Password: " << std::string(rconPass) << std::endl;
	rcon_packet.cmd = rconPass;
	rcon_packet.packetCode = 0x00;

	std::string packet;
	makePacket(rcon_packet, packet);

	std::cout << "Sending login info" << std::endl;
	std::cout << packet << std::endl;

	dgs.sendBytes(packet.data(), packet.size());

	std::cout << "Sent login info" << std::endl;

	std::clock_t start_time = std::clock();

	std::string rcvd;
	char buffer[1024];

	bool logged_in = false;
	bool cmd_sent = false;
	bool cmd_response;
	int size;


	while (true)
	{
		if ((std::clock() - start_time) / CLOCKS_PER_SEC > 10)
		{
			std::cout << "Waited more than 10 secs Exiting" << std::endl;
			break;
		}
		size = dgs.receiveFrom(buffer, sizeof(buffer)-1, sa);
		buffer[size] = '\0';
		std::cout << sa.toString() << ": " << buffer << std::endl;

		if (!logged_in && (size=!0))
		{
			if (buffer[8] == 0x01)
			{
				logged_in = true;
				std::cout << "Login Successful" << std::endl;
			}
			else if (buffer[7] == 0x02)
			{
				logged_in = true;
				std::cout << "Already logged in" << std::endl;
			}
			else if (buffer[8] == 0x00)
			{
				std::cout << "Failed Login" << std::endl;
			}
		}
		else if (cmd_sent)
		{
			if (buffer[7] == 0x01)
			{
				int sequenceNum = rcvd[8]; /*this app only sends 0x00, so only 0x00 is received*/
				if (rcvd[9] == 0x00)
				{
					int numPackets = rcvd[10];
					int packetsReceived = 0;
					int packetNum = rcvd[11];

					if ((numPackets - packetNum) == 0x01)
					{
						cmd_response = true;
					}
				}
				else
				{
					/*received command response. nothing left to do now*/
					cmd_response = true;
				}
			}
		}

		char *command = argv[2];
		if (logged_in && !cmd_sent)
		{
			std::cout << "Sending command \"" << command << "\"" << std::endl;
			rcon_packet.cmd = command;
			rcon_packet.packetCode = 0x01;

		        std::string packet;
		        makePacket(rcon_packet, packet);
		        dgs.sendBytes(packet.data(), packet.size());
		        std::cout << packet << std::endl;


			cmd_sent = true;
		}
		else if (cmd_response && cmd_sent)
		{
			/*We're done! Can exit now*/
			std::cout << "Command response received. Exiting" << std::endl;
			break;
		}

	}
	return 0;
}


// boost::this_thread::sleep(boost::posix_time::seconds(1));
