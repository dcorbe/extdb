/*
Copyright (C) 2012 Prithu "bladez" Parker <https://github.com/bladez-/bercon>
Copyright (C) 2014 Declan Ireland <http://github.com/torndeco/extDB>

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

#ifdef RCON_APP
	#include <boost/program_options.hpp>
	#include <fstream>
#endif

#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/unordered_map.hpp>

#include <Poco/Checksum.h>
#include <Poco/Net/DatagramSocket.h>
#include <Poco/Net/SocketAddress.h>
#include <Poco/Net/NetException.h>
#include <Poco/Timestamp.h>
#include <Poco/DateTimeFormatter.h>

#include <Poco/Exception.h>
#include <Poco/Stopwatch.h>

#include <Poco/AbstractCache.h>
#include <Poco/ExpireCache.h>
#include <Poco/SharedPtr.h>

#include <Poco/NumberFormatter.h>
#include <Poco/Thread.h>

#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstring>
#include <stdlib.h>

#include "rcon.h"


void Rcon::makePacket(RconPacket &rcon)
{
	Poco::Checksum checksum_crc32;

	std::ostringstream cmdStream;
	cmdStream.put(0xFFu);
	cmdStream.put(rcon.packetCode);

	if (rcon.packetCode == 0x01)
	{
		cmdStream.put(0x00); // seq number
		cmdStream << rcon.cmd;
	}
	else if (rcon.packetCode == 0x02)
	{
		cmdStream.put(rcon.cmd_char_workaround);
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
	rcon.packet = cmdPacketStream.str();
}


void Rcon::extractData(int pos, std::string &result)
{
	std::stringstream ss;
	for(size_t i = pos; i < buffer_size; ++i)
	{
	  ss << buffer[i];
	}
	result = ss.str();
}

void Rcon::mainLoop()
{
	int elapsed_seconds;
	
	bool logged_in = false;
	
	// 2 Min Cache for UDP Multi-Part Messages
	Poco::ExpireCache<int, RconMultiPartMsg > rcon_msg_cache(120000);
	
	dgs.setReceiveTimeout(Poco::Timespan(1, 0));
	while (true)
	{
		try 
		{
			buffer_size = dgs.receiveFrom(buffer, sizeof(buffer)-1, sa);
			buffer[buffer_size] = '\0';

			if (buffer[7] == 0x00)
			{
				if (buffer[8] == 0x01)
				{
					logged_in = true;
					rcon_timer.restart();
				}
				else
				{
					// Login Failed
					std::cout << "Failed Login" << std::endl;
					std::cout << "Disconnecting..." << std::endl;
					logged_in = false;
					disconnect();
					break;
				}
			}
			else if ((buffer[7] == 0x01) and logged_in)
			{
				// Rcon Server Ack Message Received
				int sequenceNum = buffer[8];
				
				// Reset Timer
				rcon_timer.restart();
				
				if (!((buffer[9] == 0x00) && (buffer_size > 9)))
				{
					// Server Received Command Msg
					std::string result;
					extractData(9, result);
					if (result.empty())
					{
						std::cout << "EMPTY" << std::endl;
					}
					else
					{
						std::cout << result << std::endl;
					}
				}
				else
				{
					// Rcon Multi-Part Message Recieved
					int numPackets = buffer[10];
					int packetNum = buffer[11];
					
					std::string partial_msg;
					extractData(12, partial_msg);
					
					RconMultiPartMsg rcon_mp_msg;

					if (!(rcon_msg_cache.has(sequenceNum)))
					{
						// Doesn't have sequenceNum in Buffer
						rcon_mp_msg.first = 1;
						rcon_msg_cache.add(sequenceNum, rcon_mp_msg);
						
						Poco::SharedPtr<RconMultiPartMsg> ptrElem = rcon_msg_cache.get(sequenceNum);
						ptrElem->second[packetNum] = partial_msg;
					}
					else
					{
						// Has sequenceNum in Buffer
						Poco::SharedPtr<RconMultiPartMsg> ptrElem = rcon_msg_cache.get(sequenceNum);
						ptrElem->first = ptrElem->first + 1;
						ptrElem->second[packetNum] = partial_msg;
						
						if (ptrElem->first == numPackets)
						{
							// All packets Received, re-construct message
							std::string result;
							for (int i = 0; i < numPackets; ++i)
							{
								result = result + ptrElem->second[i];
							}
							std::cout << result << std::endl;
							rcon_msg_cache.remove(sequenceNum);
						}
					}
				}
			}
			else if (buffer[7] == 0x02)
			{
				if (!logged_in)
				{
					// Already Logged In 
					logged_in = true;
					// Reset Timer
					rcon_timer.restart();
				}
				else
				{
					// Recieved Chat Messages
					std::string result;
					extractData(9, result);
					std::cout << "CHAT: " << result << std::endl;
					
					// Respond to Server Msgs i.e chat messages, to prevent timeout
					rcon_packet.packetCode = 0x02;
					rcon_packet.cmd_char_workaround = buffer[8];
					rcon_packet.packet.clear();
					makePacket(rcon_packet);
					dgs.sendBytes(rcon_packet.packet.data(), rcon_packet.packet.size());
					
					// Reset Timer
					rcon_timer.restart();
				}
			}

			if (logged_in)
			{
				// Checking for Commands to Send
				boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_commands);

				for(std::vector<std::string>::iterator it = rcon_commands.begin(); it != rcon_commands.end(); ++it) 
				{
					char *cmd = new char[it->size()+1] ;
					std::strcpy(cmd, it->c_str());
					
					rcon_packet.packet.clear();
					delete []rcon_packet.cmd;
					rcon_packet.cmd = cmd;
					
					rcon_packet.packetCode = 0x01;
					makePacket(rcon_packet);
					
					// Send Command
					dgs.sendBytes(rcon_packet.packet.data(), rcon_packet.packet.size());
				}
				// Clear Comands
				rcon_commands.clear();
			}
			
			{
				// Check if Run Flag Still Set
				//		Done here instead of while due to need of mutex lock
				boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_run_flag);
				if ((!rcon_run_flag) && (rcon_commands.empty()))
				{
					break;
				}
			}
		}
		catch (Poco::TimeoutException&)
		{
			boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_run_flag);
			if (!rcon_run_flag)  // Checking Run Flag
			{
				break;
			}
			else
			{
				elapsed_seconds = rcon_timer.elapsedSeconds();
				if (elapsed_seconds >= 45)
				{
					std::cout << "TIMED OUT...." << std::endl;
					disconnect();
				}
				else if (elapsed_seconds >= 30)
				{
					// Keep Alive
					//std::cout << "Keep Alive Sending" << std::endl;
					rcon_timer.restart();
					rcon_packet.packetCode = 0x01;
					rcon_packet.cmd = '\0';
					rcon_packet.packet.clear();
					makePacket(rcon_packet);
					dgs.sendBytes(rcon_packet.packet.data(), rcon_packet.packet.size());
					//std::cout << "Keep Alive Sent" << std::endl;
				}
				else if (logged_in)
				{
					// Checking for Commands to Send
					boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_commands);

					for(std::vector<std::string>::iterator it = rcon_commands.begin(); it != rcon_commands.end(); ++it) 
					{
						char *cmd = new char[it->size()+1] ;
						std::strcpy(cmd, it->c_str());
						
						rcon_packet.packet.clear();
						delete []rcon_packet.cmd;
						rcon_packet.cmd = cmd;
						
						rcon_packet.packetCode = 0x01;
						makePacket(rcon_packet);
						
						// Send Command
						dgs.sendBytes(rcon_packet.packet.data(), rcon_packet.packet.size());
					}
					// Clear Comands
					rcon_commands.clear();
				}
			}
		}
		catch (Poco::Net::ConnectionRefusedException& e)
		{
			disconnect();
			std::cout << "extDB: error rcon connect: " << e.displayText() << std::endl;
		}
		catch (Poco::Exception& e)
		{
			disconnect();
			std::cout << "extDB: error rcon: " << e.displayText() << std::endl;
		}
	}
}


void Rcon::addCommand(std::string command)
{
	boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_commands);
	rcon_commands.push_back(command);
}


void Rcon::run()
{
	connect();
	mainLoop();
}


void Rcon::connect()
{
	// Connect
	Poco::Net::SocketAddress sa(rcon_login.address, rcon_login.port);
	dgs.connect(sa);

	// Login Packet
	rcon_packet.cmd = rcon_login.password;
	rcon_packet.packetCode = 0x00;
	rcon_packet.packet.clear();
	makePacket(rcon_packet);
	dgs.sendBytes(rcon_packet.packet.data(), rcon_packet.packet.size());
	std::cout << "Sent login info" << std::endl;
	
	// Reset Timer
	rcon_timer.start();
}


void Rcon::disconnect()
{
	if (rcon_login.auto_reconnect)
	{
		connect();
	}
	else
	{
		boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_run_flag);
		rcon_run_flag = false;
	}
}


Rcon::Rcon(std::string address, int port, std::string password, bool auto_reconnect)
{
	rcon_login.address = address;
	rcon_login.port = port;
	rcon_login.auto_reconnect;
	
	char *passwd = new char[password.size()+1] ;
	std::strcpy(passwd, password.c_str());
	delete []rcon_login.password;
	rcon_login.password = passwd;
	
	boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_run_flag);
	rcon_run_flag = true;
}


#ifdef RCON_APP
int main(int nNumberofArgs, char* pszArgs[])
{
	boost::program_options::options_description desc("Options");
	desc.add_options()
		("help", "Print help messages")
		("ip", boost::program_options::value<std::string>()->required(), "IP Address for Server")
		("port", boost::program_options::value<int>()->required(), "Port for Server")
		("password", boost::program_options::value<std::string>()->required(), "Rcon Password for Server")
		("file", boost::program_options::value<std::string>(), "File to run i.e rcon restart warnings");

	boost::program_options::variables_map options;
	
	try 
	{
		boost::program_options::store(boost::program_options::parse_command_line(nNumberofArgs, pszArgs, desc), options);
		
		if (options.count("help") )
		{
			std::cout << "Rcon Command Line, based off bercon by Prithu \"bladez\" Parker" << std::endl;
			std::cout << "\t\t @ https://github.com/bladez-/bercon" << std::endl;
			std::cout << std::endl;
			std::cout << "Rewritten for extDB + crossplatform by Torndeco" << std::endl;
			std::cout << "\t\t @ https://github.com/Torndeco/extDB" << std::endl;
			std::cout << std::endl;
			std::cout << "File Option is just for parsing rcon commands to be ran, i.e server restart warnings" << std::endl;
			std::cout << "\t\t For actually restarts use a cron program to run a script" << std::endl;
			std::cout << std::endl;
			return 0;
		}
		
		boost::program_options::notify(options);
	}
	catch(boost::program_options::error& e)
	{
		std::cout << "ERROR: " << e.what() << std::endl;
		std::cout << desc << std::endl;
		return 1;
	}

	Rcon *rcon;

	Rcon rcon_runnable(options["ip"].as<std::string>(), options["port"].as<int>(), options["password"].as<std::string>(), false);
	
	Poco::Thread thread;
	thread.start(rcon_runnable);
	
	if (options.count("file"))
	{
		//std::ifstream fin(options["file"].as<std::string>());
		std::ifstream fin("test");
		if (fin.is_open() == false)
		{
			std::cout << "ERROR: file is open" << std::endl;
			return 1;
		}
		else
		{
			std::cout << "File OK" << std::endl;
		}
		
		std::string line;
		while (std::getline(fin, line))
		{
			std::cout << line << std::endl;
			if (line.empty())
			{
				boost::this_thread::sleep( boost::posix_time::milliseconds(1000) );
				std::cout << "sleep" << std::endl;
			}
			else
			{
				//std::cout << rcon1 << std::endl;
				rcon_runnable.addCommand(line);
				//std::cout << rcon2 << std::endl;
			}
		}
		std::cout << "OK" << std::endl;
		rcon_runnable.disconnect();
		thread.join();
		return 0;
	}
	else
	{
		std::cout << "**********************************" << std::endl;
		std::cout << "**********************************" << std::endl;
		std::cout << "To talk type " << std::endl;
		std::cout << "SAY -1 Server Restart in 10 mins" << std::endl;
		std::cout << std::endl;
		std::cout << "To see all players type" << std::endl;
		std::cout << "players" << std::endl;
		std::cout << "**********************************" << std::endl;
		std::cout << "**********************************" << std::endl;
		std::cout << std::endl;
		for (;;) {
			char input_str[4096];
			std::cin.getline(input_str, sizeof(input_str));
			if (std::string(input_str) == "quit")
			{
				std::cout << "Quitting Please Wait" << std::endl;
				rcon_runnable.disconnect();
				thread.join();
				break;
			}
			else
			{
				rcon_runnable.addCommand(input_str);
			}
		}
		std::cout << "Quitting" << std::endl;
		return 0;
	}
}
#endif
