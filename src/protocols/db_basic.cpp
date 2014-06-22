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
*/



#include "db_basic.h"

#include <Poco/Data/Common.h>
#include <Poco/Data/MetaColumn.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>
#include <Poco/Exception.h>

#include <cstdlib>
#include <iostream>

/*
std::string DB_BASIC::getPlayerInfo(AbstractProtocol *extension, std::string steamid)
{
	// FOO
	return "";
}

std::string DB_BASIC::newPlayerInfo(AbstractProtocol *extension, std::string steamid)
{
	// FOO
	return "";
}

std::string DB_BASIC::updatePlayerDeath(AbstractProtocol *extension, std::string steamid)
{
	// FOO
	return "";
}

std::string DB_BASIC::updatePlayerHealth(AbstractProtocol *extension, std::string steamid, std::string health)
{
	// FOO
	return "";
}

std::string DB_BASIC::updatePlayerInventory(AbstractProtocol *extension, std::string steamid, std::string inventory)
{
	// FOO
	return "";
}

std::string DB_BASIC::updatePlayerPos(AbstractProtocol *extension, std::string steamid, std::string pos)
{
	// FOO
	return "";
}
*/
std::string DB_BASIC::callProtocol(AbstractProtocol *extension, std::string input_str)
{
	/*
	const std::string sep_char(":");
	const std::string::size_type found = str_input.find(sep_char,2);

	// Data
	std::string command;
	std::string data;

	if (found==std::string::npos)  // Check Invalid Format
	{
		//command = str_input.substr(2);
	}
	else
	{
		command = str_input.substr(2,(found-2));
		data = str_input.substr(found+1);
	}

	if (command == "VERSION")
	{
		std::strcpy(output, version().c_str());
	}
	else if (command == "DATABASE")
	{
		connectDatabase(output, output_size, data); //TODO optimze return
	}
	 */
}