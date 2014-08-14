/*
Copyright (C) 2014 Declan Ireland <http://github.com/torndeco/extDB>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
 
getGUID --
Code to Convert SteamID -> BEGUID 
From Frank https://gist.github.com/Fank/11127158

*/


#include <boost/thread/thread.hpp>

#include <Poco/Checksum.h>
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DigestEngine.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/MD4Engine.h>
#include <Poco/MD5Engine.h>
#include <Poco/Timespan.h>

#include <cstdlib>
#include <iostream>

#include "../sanitize.h"

#include "abstract_ext.h"
#include "misc.h"



/*
MISC::MISC(void)
{
	checksum_adler32 = Poco::Checksum(Poco::Checksum::TYPE_ADLER32);
}*/


void MISC::getDateTime(std::string &result)
{
	Poco::DateTime now;
	result = ("[" + Poco::DateTimeFormatter::format(now, "%Y, %n, %d, %H, %M") + "]");
}


void MISC::getDateTime(int hours, std::string &result)
{
	Poco::DateTime now;
	Poco::Timespan span(hours*Poco::Timespan::HOURS);
	Poco::DateTime newtime = now + span;

	result = ("[" + Poco::DateTimeFormatter::format(newtime, "%Y, %n, %d, %H, %M") + "]");
}


void MISC::getCrc32(std::string &input_str, std::string &result)
{
	boost::lock_guard<boost::mutex> lock(mutex_checksum_crc32);
	checksum_crc32.update(input_str);
	result = ("\"" + Poco::NumberFormatter::format(checksum_crc32.checksum()) + "\"");
}


void MISC::getMD4(std::string &input_str, std::string &result)
{
	boost::lock_guard<boost::mutex> lock(mutex_md4);
	md4.update(input_str);
	result = ("\"" + Poco::DigestEngine::digestToHex(md4.digest()) + "\"");
}


void MISC::getMD5(std::string &input_str, std::string &result)
{
	boost::lock_guard<boost::mutex> lock(mutex_md5);
	md5.update(input_str);
	result = ("\"" + Poco::DigestEngine::digestToHex(md5.digest()) + "\"");
}


void MISC::getBEGUID(std::string &input_str, std::string &result)
// From Frank https://gist.github.com/Fank/11127158
// Modified to use libpoco
{
	bool status = true;
	for (unsigned int index=0; index < input_str.length(); index++)
	{
		if (!std::isdigit(input_str[index]))
		{
			status = false;
			result = "Invalid SteamID";
			break;
		}
	}
	
	if (status)
	{
		Poco::Int64 steamID = Poco::NumberParser::parse64(input_str);
		Poco::Int8 i = 0, parts[8] = { 0 };

		do
		{
			parts[i++] = steamID & 0xFFu;
		} while (steamID >>= 8);

		std::stringstream bestring;
		bestring << "BE";
		for (int i = 0; i < sizeof(parts); i++) {
			bestring << char(parts[i]);
		}

		md5.update(bestring.str());
		result = ("\"" + Poco::DigestEngine::digestToHex(md5.digest()) + "\"");
	}
}


void MISC::callProtocol(AbstractExt *extension, std::string input_str, std::string &result)
{
	// Protocol
	const std::string sep_char(":");

	std::string command;
	std::string data;

	const std::string::size_type found = input_str.find(sep_char);

	if (found==std::string::npos)  // Check Invalid Format
	{
		command = input_str;
	}
	else
	{
		command = input_str.substr(0,found);
		data = input_str.substr(found+1);
	}
	if (command == "TIME")
	{
		if (data.length() > 0)
		{
			getDateTime(Poco::NumberParser::parse(data), result); //TODO try catch or insert number checker function
		}
		else
		{
			getDateTime(result);
		}
	}
	else if (command == "BEGUID")
	{
		getBEGUID(data, result);
	}
	else if (command == "CRC32")
	{
		getCrc32(data, result);
	}
	else if (command == "MD4")
	{
		getMD4(data, result);
	}
	else if (command == "MD5")
	{
		getMD5(data, result);
	}
	else if (command == "TEST")
	{
		result = data;
	}
	else
	{
		result = ("[0,\"Error Invalid Command\"]");
	}
}