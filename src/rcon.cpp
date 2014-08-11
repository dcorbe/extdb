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

#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>

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


void Rcon::mainLoop()
{
	dgs.setReceiveTimeout(Poco::Timespan(1, 0));
	while (true)
	{
		try 
		{
			size = dgs.receiveFrom(buffer, sizeof(buffer)-1, sa);
			buffer[size] = '\0';
			buffer_size = size;
			
			if (buffer[7] == 0x02)
			{
				if (!logged_in)
				{
					logged_in = true;
					//std::cout << "Already logged in" << std::endl;
					//std::cout << "reset timer" << std::endl;
					rcon_timer.restart();
				}
				else
				{
					std::string result;
					extractData(9, std::string(buffer), result);
					//std::cout << "CHAT: " << result << std::endl;
					
					// Respond to Server Msgs i.e chat messages, to prevent timeout
					rcon_packet.packetCode = 0x02;
					rcon_packet.cmd_char_workaround = buffer[8];
					rcon_packet.packet.clear();

					makePacket(rcon_packet);
					dgs.sendBytes(rcon_packet.packet.data(), rcon_packet.packet.size());
					//std::cout << "reset timer" << std::endl;
					rcon_timer.restart();
				}
			}

			if (!logged_in)
			{
				if (buffer[8] == 0x01) // Login Successful
				{
					logged_in = true;
					//std::cout << "reset timer" << std::endl;
					rcon_timer.restart();
				}
				else if (buffer[8] == 0x00) // Login Failed
				{
					std::cout << "Failed Login" << std::endl;
					break;
				}
			}
			else if (cmd_sent)
			{
				/*
				std::cout << "-----------------------------------------------------" << std::endl;
				std::cout << "DEBUG: " << Poco::NumberFormatter::format(int(buffer[7])) << std::endl;
				std::cout << "DEBUG: " << Poco::NumberFormatter::format(int(buffer[8])) << std::endl;
				std::cout << "DEBUG: " << Poco::NumberFormatter::format(int(buffer[9])) << std::endl;
				std::cout << "DEBUG: " << Poco::NumberFormatter::format(int(buffer[10])) << std::endl;
				std::cout << "DEBUG: " << Poco::NumberFormatter::format(int(buffer[11])) << std::endl;
				std::cout << "DEBUG: BUFFER SIZE: " << Poco::NumberFormatter::format(buffer_size) << std::endl;
				*/
				
				if (buffer[7] == 0x01)
				{
					Poco::ExpireCache<int, RconMultiPartMsg > rcon_msg_cache(120000); 
					
					//std::cout << "Buffer 7 = 0x01" << std::endl;
					int sequenceNum = buffer[8]; // Store in Poco Map Expire time based vector  <Packets Received> / <Number of Packets> / array[num of packets] of std::string>
					if ((buffer[9] == 0x00) && (buffer_size > 9))
					{
						//std::cout << "Buffer 9 = 0x00" << std::endl;
						int numPackets = buffer[10];
						int packetsReceived = 0;
						int packetNum = buffer[11];
						
						std::string partial_msg;
						extractData(12, std::string(buffer), partial_msg);
						
						RconMultiPartMsg rcon_mp_msg;
						
						if !(rcon_msg_cache.has(sequenceNum))  //SharedPtr::isNull()
							// Has Seq in Buffer
						{
							std::cout << "Multi Msg Message part" << std::endl;
							Poco::SharedPtr<RconMultiPartMsg> ptrElem = rcon_msg_cache.get(sequenceNum);
							ptrElem->first =+ 1; // packetsReceived
							ptrElem->second[packetNum] = partial_msg;
							
							if (packetsReceived == numPackets)
							{
								// Build String together
								std::string result;
								for (std::vector<std::string>::iterator it = ptrElem->second.begin(); it != ptrElem->second.end(); ++it)
								{
									result =+ it->c_str();
								}
								std::cout << "Multi Msg Message Built" << std::endl;
								rcon_msg_cache.remove(sequenceNum);
								cmd_response = true;
							}
						}
						rcon_timer.restart();
					}
					else
					{
						/*received command response.*/
						//std::cout << "Command response received" << std::endl;
						cmd_response = true;
						rcon_timer.restart();
					}
				}
				if (cmd_response)
				{
					/*received command response.*/
					//std::cout << "Command response received" << std::endl;
					rcon_timer.restart();
				}
				//std::cout << "-----------------------------------------------------" << std::endl;
			}
			else if (logged_in)
			{
				// Mutex Lock
				boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_commands);

				// Commands To Send
				for(std::vector<std::string>::iterator it = rcon_commands.begin(); it != rcon_commands.end(); ++it) 
				{
					char *cmd = new char[it->size()+1] ;
					std::strcpy(cmd, it->c_str());
					
					rcon_packet.packet.clear();
					rcon_packet.cmd = cmd;
					rcon_packet.packetCode = 0x01;
					makePacket(rcon_packet);
					
					// Send Command
					dgs.sendBytes(rcon_packet.packet.data(), rcon_packet.packet.size());
					cmd_sent = true;
				}
				rcon_commands.clear();
			}
			{
				boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_run_flag);
				if (!rcon_run_flag)
				{
					break;
				}
			}
		}
		catch (Poco::TimeoutException&)
		{
			elapsed_seconds = rcon_timer.elapsedSeconds();
			if (elapsed_seconds >= 45)
			{
				std::cout << "TIMED OUT" << std::endl;
			}
			else if (elapsed_seconds >= 30)
			{
				// Keep Alive
				std::cout << "Keep Alive Sending" << std::endl;
				rcon_packet.packetCode = 0x01;
				rcon_packet.cmd = '\0';
				rcon_packet.packet.clear();
				makePacket(rcon_packet);
				dgs.sendBytes(rcon_packet.packet.data(), rcon_packet.packet.size());
				std::cout << "Keep Alive Sent" << std::endl;
			}
			else if (logged_in)
			{
				//Mutex Lock
				boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_commands);
				// Commands
				if (rcon_commands.size() > 0)
				{
					for(std::vector<std::string>::iterator it = rcon_commands.begin(); it != rcon_commands.end(); ++it) 
					{
						char *cmd = new char[it->size()+1] ;
						std::strcpy(cmd, it->c_str());
						
						rcon_packet.packet.clear();
						rcon_packet.cmd = cmd;
						rcon_packet.packetCode = 0x01;
						makePacket(rcon_packet);
						
						// Send Command
						dgs.sendBytes(rcon_packet.packet.data(), rcon_packet.packet.size());
						cmd_sent = true;
					}
					rcon_commands.clear();
				}
			}
		}
		catch (Poco::Net::ConnectionRefusedException& e)
		{
			std::cout << "extDB: rcon connect Failed: " << e.displayText() << std::endl;
			//ADD CODE TO ATTEMPT RECONNECTS
		}
		catch (Poco::Exception& e)
		{
			std::cout << "extDB: rcon Failed: " << e.displayText() << std::endl;
		}
	}
}


void Rcon::extractData(int pos, std::string data, std::string &result)
{
	std::stringstream ss;
	for(size_t i = pos; i < size; ++i)
	{
	  ss << buffer[i];
	}
	result = ss.str();
}


void Rcon::connect()
{
	Poco::Net::SocketAddress sa("localhost", rcon_login.port);

	dgs.connect(sa);

	std::cout << "Password: " << std::string(rcon_login.password) << std::endl;

	rcon_packet.cmd = rcon_login.password;
	rcon_packet.packetCode = 0x00;
	rcon_packet.packet.clear();

	makePacket(rcon_packet);

	std::cout << "Sending login info" << std::endl;

	dgs.sendBytes(rcon_packet.packet.data(), rcon_packet.packet.size());

	rcon_timer.start();

	logged_in = false;
	cmd_sent = false;
	cmd_response = false;
}


Rcon::Rcon(int port, std::string password)
{
	char *passwd = new char[password.size()+1] ;
	std::strcpy(passwd, password.c_str());
	rcon_login.port = port;
	rcon_login.password = passwd;
	
	boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_run_flag);
	rcon_run_flag = true;
}


void Rcon::run()
{
	std::cout << "RCON Starting" << std::endl;
	connect();
	mainLoop();
}

void Rcon::disconnect()
{
	{
		boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_run_flag);
		rcon_run_flag = false;
	}
}

void Rcon::addCommand(std::string command)
{
	boost::lock_guard<boost::recursive_mutex> lock(mutex_rcon_commands);
	rcon_commands.push_back(command);
}

//mutex_rcon_global

#ifdef RCON_APP
int main(int nNumberofArgs, char* pszArgs[])
{
	Rcon *rcon;
	int port = atoi(pszArgs[1]);
	std::string password = pszArgs[2];
	
	
	//rcon = (new Rcon()); 
	//rcon->init(port, password);
	//rcon->sendCommand(pszArgs[3]);
	
	Rcon rcon_runnable(port, password);
	
	Poco::Thread thread;
	thread.start(rcon_runnable);
	//thread.join();
	
	char result[255];
	
	for (;;) {
		char input_str[100];
		std::cin.getline(input_str, sizeof(input_str));
		if (std::string(input_str) == "quit")
		{
			std::cout << "Quiting Please Wait" << std::endl;
			rcon_runnable.disconnect();
			thread.join();
			break;
		}
		else
		{
			rcon_runnable.addCommand(input_str);
		}
	}
	std::cout << "quitting" << std::endl;
	return 0;
}
#endif
