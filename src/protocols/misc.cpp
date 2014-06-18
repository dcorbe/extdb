/*
* Copyright (C) 2014 Declan Ireland <http://github.com/torndeco/extDB>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/

#include "misc.h"

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

#include "../ext.h"

/*
MISC::MISC(void)
{
	checksum_adler32 = Poco::Checksum(Poco::Checksum::TYPE_ADLER32);
}*/


std::string MISC::getDateTime()
{
	Poco::DateTime now;
	return ("[" + Poco::DateTimeFormatter::format(now, "%Y, %n, %d, %H, %M") + "]");
}

std::string MISC::getDateTime(int hours)
{
	Poco::DateTime now;
	/*
	Poco::DateTime newtime(
		now.year(),
		now.month(),
		now.day(),
		(now.hour() + hours),
		now.minute(),
		now.second());
	*/
	Poco::Timespan span(hours*Poco::Timespan::HOURS);
	Poco::DateTime newtime = now + span;

	return ("[" + Poco::DateTimeFormatter::format(newtime, "%Y, %n, %d, %H, %M") + "]");
}

/*
std::string MISC::getAdler32(std::string &str_input)
{
	boost::lock_guard<boost::mutex> lock(mutex_checksum_adler32);
	checksum_adler32.update(str_input);
	return Poco::NumberFormatter::format(checksum_adler32.checksum());
}
*/

std::string MISC::getCrc32(std::string &str_input)
{
	boost::lock_guard<boost::mutex> lock(mutex_checksum_crc32);
	checksum_crc32.update(str_input);
	return ("\"" + Poco::NumberFormatter::format(checksum_crc32.checksum()) + "\"");
}

std::string MISC::getMD4(std::string &str_input)
{
	boost::lock_guard<boost::mutex> lock(mutex_md4);
	md4.update(str_input);
	return ("\"" + Poco::DigestEngine::digestToHex(md4.digest()) + "\"");
}

std::string MISC::getMD5(std::string &str_input)
{
	boost::lock_guard<boost::mutex> lock(mutex_md5);
	md5.update(str_input);
	return ("\"" + Poco::DigestEngine::digestToHex(md5.digest()) + "\"");
}

std::string MISC::callProtocol(AbstractExt *extension, std::string str_input)
{
	// Protocol
	const std::string sep_char(":");

	std::string command;
	std::string data;
	std::string result;

	const std::string::size_type found = str_input.find(sep_char);

	if (found==std::string::npos)  // Check Invalid Format
	{
		command = str_input;
	}
	else
	{
		command = str_input.substr(0,found);
		data = str_input.substr(found+1);
		std::cout << "DEBUG MISC: command: " << command << "." << std::endl;
		std::cout << "DEBUG MISC: data: " << data << "." << std::endl;
	}

	if (command == "TIME")
	{
		if (data.length() > 0)
		{
			result = getDateTime(Poco::NumberParser::parse(data)); //TODO try catch
		}
		else
		{
			result = getDateTime();
		}
	}
//	else if (command == "ADLER32")
//	{
//		result = getAdler32(data);
//	}
	else if (command == "CRC32")
	{
		result = getCrc32(data);
	}
	else if (command == "MD4")
	{
		result = getMD4(data);;
	}
	else if (command == "MD5")
	{
		result = getMD5(data);;
	}
	else if (command == "TEST")
	{
		result = data;
	}
	else
	{
		result = ("[\"ERROR\",\"Error Invalid Command\"]");
	}
	return result;
}