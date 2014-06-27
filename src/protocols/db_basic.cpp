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

#include <Poco/DateTime.h>
#include <Poco/DateTimeFormatter.h>

#include <Poco/Data/Common.h>
#include <Poco/Data/MetaColumn.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>
#include <Poco/Exception.h>

#include <cstdlib>
#include <iostream>


void DB_BASIC::getCharUID(Poco::Data::Session &db_session, std::string &steamid, std::string &result)
{
	std::string name = "deco";
	Poco::DateTime now;
	std::string timestamp = Poco::DateTimeFormatter::format(now, "\"[%Y, %n, %d, %H, %M]\"");
	
	Poco::Data::Statement sql1(db_session);
	sql1 << ("SELECT \"Char UID\" FROM `Player Info` WHERE SteamID=" + steamid), Poco::Data::into(result), Poco::Data::now;
	//sql.execute();

	if (result.empty())
	{
		std::cout << "NEW PLAYER" << std::endl;
		// TODO: Performance look @ implementing MariaDB + SQLite c library directly so can get last row id directly from database handle.
		Poco::Data::Statement sql2(db_session);
		std::cout << "INSERT INTO \"Player Characters\" (SteamID, `Alive`, `First Updated`, `Last Updated`) VALUES (" + steamid + ", 0, " + timestamp + ", " + timestamp + ")" << std::endl;
		sql2 << ("INSERT INTO \"Player Characters\" (SteamID, `Alive`, `First Updated`, `Last Updated`) VALUES (" + steamid + ", 0, " + timestamp + ", " + timestamp + ")");
		sql2.execute();
		Poco::Data::Statement sql3(db_session);
		std::cout << "SELECT `UID` FROM `Player Characters` WHERE `SteamID`=" + steamid << std::endl;
		sql3 << ("SELECT `UID` FROM `Player Characters` WHERE `SteamID`=" + steamid), Poco::Data::into(result), Poco::Data::now;
		//sql.execute();
		Poco::Data::Statement sql4(db_session);
		std::string test = ("INSERT INTO `Player Info` (SteamID, Name, `First Login`, `Last Login`, `Char UID`) VALUES (" + steamid + ", " + name + ", " + timestamp + ", " + timestamp + ", " + result + ")");
		std::cout << test << std::endl;
		sql4 << ("INSERT INTO \"Player Info\" (SteamID, Name, `First Login`, `Last Login`, `Char UID`) VALUES (" + steamid + ", \"" + name + "\", " + timestamp + ", " + timestamp + ", " + result + ")");
		sql4.execute();
	}
	else
	{
		std::cout << "OLD PLAYER" << std::endl;
		Poco::Data::Statement sql5(db_session);
		sql5 << ("UPDATE \"Player Info\" SET `Last Login` = " + timestamp + " WHERE SteamID=" + steamid, Poco::Data::now);
		//sql5.execute();
	}
}


void DB_BASIC::getOptionAll(Poco::Data::Session &db_session, std::string &table, std::string &result)
{
	Poco::Data::Statement sql(db_session);
	sql << ("SELECT * FROM `" + table + "` AND Alive = 1");
	sql.execute();

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
}


void DB_BASIC::getOption(Poco::Data::Session &db_session, std::string &table, std::string &uid, std::string &option, std::string &result)
{
	Poco::Data::Statement sql(db_session);
	sql << ("SELECT \"UID\" FROM \"" + table + "\" WHERE " + option + "UID=" + uid + "", Poco::Data::into(result), Poco::Data::now);
	sql.execute();
}


void DB_BASIC::setOption(Poco::Data::Session &db_session, std::string &table, std::string &uid, std::string &option, std::string &value, std::string &result)
{
	Poco::Data::Statement sql(db_session);
	sql << ("UPDATE \"" + table + "\" SET `" + option + "` = " + value + " WHERE UID=" + uid);
	sql.execute();
	result = "[\"OK\",\"OK\"]";; // TODO
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

std::string DB_BASIC::callProtocol(AbstractExt *extension, std::string input_str)
{
	std::cout << "DEBUG: " << input_str << std::endl;
	std::string result;
	if (input_str.length() <= 5)
	{
		result = "[\"ERROR\",\"Error Invalid Message\"]";
	}
	else
	{
		bool option_all;
		std::string option;
		std::string value;
		
		const std::string sep_char(":");
		const std::string::size_type found = input_str.find(sep_char,4);
		
		if (found==std::string::npos)  // Check Invalid Format
		{
			result = "[\"ERROR\",\"Error\"]"; // TODO
		}
		else
		{
			std::string uid = input_str.substr(4,found-4);
			std::string value = input_str.substr(found+1);
			
			std::cout << "DEBUG 1: " << uid << std::endl;
			std::cout << "DEBUG 2: " << value << std::endl;
			
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
				default:
					option = "Model";
			}
			
			switch (Poco::NumberParser::parse(input_str.substr(1,1)))
			{
				case (0):  // Player Info
				{
					if (option =="Other")
					{
						std::string table = "Player Info";
						if ((Poco::NumberParser::parse(input_str.substr(0,1))) == 5)
						{
							getOption(db_session, table, uid, option, result);
						}
						else
						{
							setOption(db_session, table, uid, option, value, result);
						}
					}
					else
					{
						getCharUID(db_session, value, result);
					}
				}
				case (1):  // Player Char  "Player Characters"
				{
					std::string table = "Player Characters";
					if ((Poco::NumberParser::parse(input_str.substr(0,1))) == 5)
					{
						getOption(db_session, table, uid, option, result);
					}
					else
					{
						setOption(db_session, table, uid, option, value, result);
					}
					break;
				}
				case (2):  // Vehicles    "Vehicles"
				{
					std::string table = "Vehicles";
					if ((Poco::NumberParser::parse(input_str.substr(0,1))) == 5)
					{
						if (option_all)
						{
							getOptionAll(db_session, table, result);
						}
						else
						{
							getOption(db_session, table, uid, option, result);
						}
					}
					else
					{
						setOption(db_session, table, uid, option, value, result);
					}
					break;
				}
				case (3):  // Objects    "Objects"
				{
					std::string table = "Objects";
					if ((Poco::NumberParser::parse(input_str.substr(0,1))) == 5)
					{
						if (option_all)
						{
							getOptionAll(db_session, table, result);
						}
						else
						{
							getOption(db_session, table, uid, option, result);
						}
					}
					else
					{
						setOption(db_session, table, uid, option, value, result);
					}
					break;
				}
				default:
					result = "[\"ERROR\",\"Error\"]";
			}
		}
	}
	return result;
}
