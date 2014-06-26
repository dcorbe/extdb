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


void DB_BASIC::getCharUID(std::string &steamid, std::string &result)
{
	select << ("SELECT \"Char UID\" FROM `PLAYER Info` where SteamID=" + steamid), Poco::Data::into(result), Poco::Data::now;
	select.execute();

	if (char_uid.empty())
	{
		// TODO: Performance look @ implementing MariaDB + SQLite c library directly so can get last row id directly from database handle.
		select << ("INSERT INTO \"Player Characters\"(SteamID, `First Updated`) VALUES (" + steamid + ", " + id + ")");
		select.execute();
		select << ("SELECT \"UID\" FROM \"Player Characters\" WHERE SteamID=" + steamid + " AND Alive = 1 ORDER BY UID DESC LIMIT 1", Poco::Data::into(result), Poco::Data::now);
		select.execute();
		select << ("UPDATE \"Player Info\" SET `Char UID` = " + result + " WHERE SteamID=" + steamid);
		select.execute();
	}
	else
	{
		char_uid = rs.value("Char UID").convert<std::string>();
		select << ("UPDATE \"Player Info\" SET `Last Login` = " + timestamp + " WHERE SteamID=" + steamid);
		select.execute();
	}
	return char_uid;
}


void DB_BASIC::getOptionAll(std::string &table)
{
	sql << ("SELECT * FROM `" + table + "` AND Alive = 1");
	sql.execute();

	std::string result;
	Poco::Data::RecordSet rs(sql);
	
	std::size_t cols = rs.columnCount();
	if (cols >= 1)
	{
		bool more = rs.moveFirst();
		while (more)
		{
			result += " [";
			for (std::size_t col = 0; col < cols; ++col)
			{
				if (rs.columnType(col) == Poco::Data::MetaColumn::FDT_STRING)
				{
					result += "\"" + (rs[col].convert<std::string>() + "\"" + ", ");
				}
				else
				{
					result += (rs[col].convert<std::string>() + ", ");
				}
			}

			more = rs.moveNext();
			if (more)
			{
				result += "],";
			}
			else
			{
				result = result.substr(0, (result.length() - 2)) + "]";
			}
		}
	}
	return result;
}


void DB_BASIC::getOption(std::string &table, std::string &uid)
{
	sql << ("SELECT \"UID\" FROM \"" + table + "\" WHERE " + option + "UID=" + uid + "", Poco::Data::into(result), Poco::Data::now);
	sql.execute();
	return result;
}


void DB_BASIC::setOption(std::string &table, std::string &uid)
{
	sql << ("UPDATE \"" + table + "\" SET `" + option + "` = " + value + " WHERE UID=" + uid);
	sql.execute();
	return result;
}


/*
Player Info ->	0 6
 * 

Player				1
Vehicle				2
Objects				3

All					0
Model				1
Position			2
Inventory			3
Medical				4
Alive				5
Other				6
Everything alive	9

Get					5
Save		0-		2
*/

//setValue(table, uid, type, value)
//getValue(table, uid, type, value)

std::string DB_BASIC::callProtocol(AbstractProtocol *extension, std::string input_str)
{
	std::string result;
	if (str_input.length() <= 4)
	{
		result = "[\"ERROR\",\"Error Invalid Message\"]";
	}
	else
	{
		const std::string sep_char(":");
		std::string option;
		bool option_all;
		
		Poco::Data::Session db_session = extension->getDBSession_mutexlock();
		
		switch (Poco::NumberParser::parse(input_str.substr(2,1)))
		{
			case 0:
				option = "Model";
				option_all = true;
				break;
			case 1:
				option = "Model";
				break;
			case 2:
				option = "Position";
				break;
			case 3:
				option = "Inventory";
				break;
			case 4:
				option = "Medical";
				break;
			case 5:
				option = "Alive";
				break;
			case 6:
				option = "Other";
				break;
			default
				option = "Model";
		}
		
		switch (Poco::NumberParser::parse(input_str.substr(1,1)))
		{
			case (0):  // Player Info
			{
				if (option =="Other")
				{
					if ((Poco::NumberParser::parse(input_str.substr(0,1))) == 5)
					{
						getOption("Player Info", uid, option, result);
					}
					else
					{
						setOption("Player Info", uid, option, value);
					}
				}
				else
				{
					getCharUID(steamid, result);
				}
			}
			case (1):  // Player Char  "Player Characters"
			{
				if ((Poco::NumberParser::parse(input_str.substr(0,1))) == 5)
				{
					getOption("Player Characters", uid, option, result);
				}
				else
				{
					setOption("Player Characters", uid, option, value, result);
				}
				break;
			}
			case (2):  // Vehicles    "Vehicles"
			{
				if ((Poco::NumberParser::parse(input_str.substr(0,1))) == 5)
				{
					if (option_all)
					{
						getOptionAll("Vehicles", result);
					}
					else
					{
						getOption("Vehicles", uid, option, result);
					}
				}
				else
				{
					setOption("Vehicles", uid, option, value, result);
				}
				break;
			}
			case (3):  // Objects    "Objects"
			{
				if ((Poco::NumberParser::parse(input_str.substr(0,1))) == 5)
				{
					if (option_all)
					{
						getOptionAll("Objects", result);
					}
					else
					{
						getOption("Objects", uid, option, result);
					}
				}
				else
				{
					setOption("Objects", uid, option, value, result);
				}
				break;
			}
			default:
				result = "[\"ERROR\",\"Error\"]";
		}
	}
	return result;
}