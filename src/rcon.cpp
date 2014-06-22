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
 * Changed Code to use Poco Net Library & Poco Checksums
*/


#include <Poco/Checksum.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/NetException.h>
#include <Poco/Timestamp.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Exception.h>
#include <Poco/Timespan.h>

#include <sstream>
#include <iomanip>
#include <iostream>

#include "rcon.h"

void Rcon::makePacket(RConPacket rcon, std::string &cmdPacket)
{
	Poco::Checksum checksum_crc32;

	std::ostringstream cmdStream;
	cmdStream.put(0xFF);
	cmdStream.put(rcon.packetCode);

	if (rcon.packetCode == 0x01)
	{
		cmdStream.put(0x00); // seq number
		cmdStream << rcon.cmd;
	}
	else if (rcon.packetCode == 0x02)
	{
		cmdStream.put(rcon.cmd);
	}
	else
	{
		cmdStream << rcon.cmd;
	}

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

void Rcon::connect(int &port, std::string &password)
{
	Poco::Net::SocketAddress sa("localhost", port);

	dgs.connect(sa);

	std::cout << "Password: " << std::string(password) << std::endl;
	rcon_packet.cmd = password.c_str();
	rcon_packet.packetCode = 0x00;

	std::string packet;
	makePacket(rcon_packet, packet);

	std::cout << "Sending login info" << std::endl;
	std::cout << packet << std::endl;

	dgs.sendBytes(packet.data(), packet.size());

	std::cout << "Sent login info" << std::endl;

	start_time = std::clock();

	logged_in = false;
	cmd_sent = false;
	cmd_response = false;
}

void Rcon::sendCommand(std::string &command, std::string &response)
{
	dgs.setReceiveTimeout(Poco::Timespan(30, 0));
	while (true)
	{
		std::cout << ".";
		if ((std::clock() - start_time) / CLOCKS_PER_SEC > 10)
		{
			std::cout << "Waited more than 10 secs Exiting" << std::endl;
			break;
		}
		try 
		{
			size = dgs.receiveFrom(buffer, sizeof(buffer)-1, sa);
			std::cout << "." << std::endl;
			buffer[size] = '\0';
			std::cout << sa.toString() << ": " << buffer << std::endl;
			std::cout << "size:" << size << std::endl;

			if (buffer[7] == 0x02)
			{
				if (!logged_in)
				{
					logged_in = true;
					std::cout << "Already logged in" << std::endl;
				}
				else 
				{
					std::cout << "Server Msg ID:" << buffer [8] << std::endl;
					rcon_packet.packetCode = 0x02;
					rcon_packet.cmd = buffer[8];
					std::string packet;
					makePacket(rcon_packet, packet);
					dgs.sendBytes(packet.data(), packet.size());
					std::cout << packet << std::endl;
					std::cout << std::endl;
					
					std::stringstream ss;
					for(size_t i = 9; i < size; ++i)
					{
					  ss << buffer[i];
					}
					std::string s = ss.str();
					std::cout << s << std::endl;
					
				}
			}
			
			if (!logged_in)
			{
				if (buffer[8] == 0x01)
				{
					logged_in = true;
					std::cout << "Login Successful" << std::endl;
				}
				else if (buffer[8] == 0x00)
				{
					std::cout << "Failed Login" << std::endl;
				}
			}
			else if (cmd_sent)   // FINISH !!!!!
			{
				if (buffer[7] == 0x01)
				{
					//int sequenceNum = buffer[8]; /*this app only sends 0x00, so only 0x00 is received*/ // TODO: Add recieve Chat from Server  http://www.battleye.com/downloads/BERConProtocol.txt
					if (buffer[9] == 0x00)
					{
						int numPackets = buffer[10];
						int packetsReceived = 0;
						int packetNum = buffer[11];

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
				if (cmd_response)
				{
					/*We're done! Can exit now*/
					std::cout << "Command response received. Exiting" << std::endl;
					//break;
				}
			}
			else if (logged_in)
			{
				std::cout << "Sending command \"" << command << "\"" << std::endl;
				rcon_packet.cmd = command.c_str();
				rcon_packet.packetCode = 0x01;
				std::string packet;
				makePacket(rcon_packet, packet);
				dgs.sendBytes(packet.data(), packet.size());
				std::cout << packet << std::endl;
				cmd_sent = true;
			}
		}
		catch (Poco::TimeoutException&)
		{
			std::cout << "Sending KeepAlive" << std::endl;
			rcon_packet.packetCode = 0x01;
			rcon_packet.cmd = "";
			std::string packet;
			makePacket(rcon_packet, packet);
			dgs.sendBytes(packet.data(), packet.size());
			std::cout << "Sent KeepAlive" << std::endl;
		}
	}
}

void Rcon::sendCommand(int port, std::string password, std::string command)
{
	try
	{
		std::string response;
		connect(port, password);
		sendCommand(command, response);
		std::cout << response << std::endl;
	}
//	catch (Poco::Net::ConnectionRefusedException& e)
//	{
//		std::cout << "extDB: rcon connect Failed: " << e.displayText() << std::endl;
//	}
	catch (Poco::Exception& e)
	{
		std::cout << "extDB: rcon Failed: " << e.displayText() << std::endl;
//		std::exit(EXIT_FAILURE);
	}
}

// boost::this_thread::sleep(boost::posix_time::seconds(1));



int main(int nNumberofArgs, char* pszArgs[])
{
    Rcon *rcon;
    rcon = (new Rcon());
    char result[255];
    for (;;) {
        char input_str[100];
		std::cin.getline(input_str, sizeof(input_str));
        if (std::string(input_str) == "quit")
        {
            break;
        }
        else
        {
            rcon->sendCommand(int(2302), std::string("testing"), std::string(input_str));
            //std::cout << "extDB: " << result << std::endl;
        }
    }
	std::cout << "quitting" << std::endl;
    return 0;
}
