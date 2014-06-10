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

#include "abstractplugin.h"

#include <boost/thread/thread.hpp>

#include <Poco/Checksum.h>
#include <Poco/ClassLibrary.h>
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>

#include <cstdlib>
#include <iostream>

std::string MISC::getDateTime()
{
    Poco::DateTime now;
    return ("[" + Poco::DateTimeFormatter::format(now, "%Y, %n, %d, %H, %M") + "]");
}

std::string MISC::getDateTime(int hours)
{
    Poco::DateTime now;
	Poco::DateTime newtime(
		now.year(),
		now.month(),
		now.day(),
		(now.hour() + hours),
		now.minute(),
		now.second());
	
    return ("[" + Poco::DateTimeFormatter::format(newtime, "%Y, %n, %d, %H, %M") + "]");
}

std::string MISC::getCrc32(std::string &str_input)
{
	boost::lock_guard<boost::mutex> lock(mutex_checksum_crc32);
    checksum_crc32.update(str_input);
    return Poco::NumberFormatter::format(checksum_crc32.checksum());
}

std::string MISC::callPlugin(AbstractExt *extension, std::string str_input)
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
    else if (command == "CRC32")
    {
        result = getCrc32(data);
    }
    else if (command == "MD5")
    {
        //msg = connectDatabase(data);
    //    std::strcpy(output, msg.c_str());
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

POCO_BEGIN_MANIFEST(AbstractPlugin)
	POCO_EXPORT_CLASS(MISC)
POCO_END_MANIFEST

// optional set up and clean up functions
void pocoInitializeLibrary()
{
	//std::cout << "Plugin DB_RAW Library initializing" << std::endl;
}

void pocoUninitializeLibrary()
{
	//std::cout << "Plugin DB_RAW Library uninitializing" << std::endl;
}
