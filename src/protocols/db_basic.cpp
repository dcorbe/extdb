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

#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Data/SQLite/Connector.h>
#include <Poco/Data/SQLite/SQLiteException.h>
#include <Poco/Data/ODBC/Connector.h>
#include <Poco/Data/ODBC/ODBCException.h>

#include <cctype>
#include <cstdlib>
#include <iostream>

#include "../sanitize.h"


bool DB_BASIC::init(AbstractExt *extension, const std::string init_str)
{
	if (extension->getDBType() == std::string("MySQL"))
	{
		return true;
	}
	else if (extension->getDBType() == std::string("ODBC"))
	{
		return true;
	}
	else if (extension->getDBType() == std::string("SQLite"))
	{
		return true;
	}
	else
	{
		// DATABASE NOT SETUP YET
		#ifdef TESTING
			std::cout << "extDB: DB_BASIC: No Database Connection" << std::endl;
		#endif
		
		BOOST_LOG_SEV(logger, boost::log::trivial::fatal) << "extDB: DB_BASIC: No Database Connection";
		return false;
	}
}


bool DB_BASIC::isNumber(std::string &input_str)
{
	bool status = true;
	for (unsigned int index=0; index < input_str.length(); index++)
	{
		if (!std::isdigit(input_str[index]))
		{
			status = false;
			break;
		}
	}
	return status;
}

void DB_BASIC::getCharUID(Poco::Data::Session &db_session, std::string &steamid, std::string &result)
{
	if (isNumber(steamid))
	{
		// TODO check user input + grab name
		std::string name = "NOT IMPLEMENTED YET";
		Poco::DateTime now;
		std::string timestamp = Poco::DateTimeFormatter::format(now, "'[%Y, %n, %d, %H, %M]'");
		
		Poco::Data::Statement sql1(db_session);
		sql1 << ("SELECT `Char UID` FROM `Player Info` WHERE SteamID=" + steamid), Poco::Data::into(result), Poco::Data::now;

		if (result.empty())
		{
			// TODO: Performance look @ implementing MariaDB + SQLite c library directly so can get last row id directly from database handle.
			Poco::Data::Statement sql2(db_session);
			sql2 << ("INSERT INTO `Player Characters` (SteamID, `Alive`, `First Updated`, `Last Updated`) VALUES (" + steamid + ", 0, " + timestamp + ", " + timestamp + ")"), Poco::Data::now;
			
			Poco::Data::Statement sql3(db_session);
			sql3 << ("SELECT `UID` FROM `Player Characters` WHERE `SteamID`=" + steamid), Poco::Data::into(result), Poco::Data::now;
			
			Poco::Data::Statement sql4(db_session);
			sql4 << ("INSERT INTO `Player Info` (SteamID, Name, `First Login`, `Last Login`, `Char UID`) VALUES (" + steamid + ", '" + name + "', " + timestamp + ", " + timestamp + ", " + result + ")"), Poco::Data::now;
		}
		else
		{
			Poco::Data::Statement sql5(db_session);
			sql5 << ("UPDATE `Player Info` SET `Last Login` = " + timestamp + " WHERE SteamID=" + steamid), Poco::Data::now;
			
			Poco::Data::Statement sql6(db_session);
			sql6 << ("UPDATE `Player Info` SET Name = '" + name + "' WHERE SteamID=" + steamid), Poco::Data::now;
		}
		result = "[1, " + result + "]";
	}
	else
	{
		result = "[0, \"ERROR UID\"]";
	}
}


void DB_BASIC::getOptionAll(Poco::Data::Session &db_session, std::string &table, std::string &result)
{
	Poco::Data::Statement sql(db_session);
	sql << ("SELECT * FROM `" + table + "` WHERE Alive = 1"), Poco::Data::now;

	Poco::Data::RecordSet rs(sql);
	
	result = "[";
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
					if (!rs[col].isEmpty())
					{
						result += "\"" + (rs[col].convert<std::string>() + "\"");
					}
					else
					{
						result += ("\"\"");
					}
				}
				else
				{
					if (!rs[col].isEmpty())
					{
						result += rs[col].convert<std::string>();
					}
				}
				if (col < (cols - 1))
				{
					result += ", ";
				}
			}
			more = rs.moveNext();
			if (more)
			{
				result += "],";
			}
			else
			{
				result += "]";
			}
		}
	}
	result += "]";
}


void DB_BASIC::getOption(Poco::Data::Session &db_session, std::string &table, std::string &uid, std::string &option, std::string &result)
{
	if (isNumber(uid))
	{
		Poco::Data::Statement sql(db_session);
		sql << ("SELECT `" + option + "` FROM `" + table + "` WHERE UID=" + uid), Poco::Data::into(result), Poco::Data::now;
		result = "[1, [" + result + "]]";
		if (!Sqf::check(result))
		{
			result = "[0, \"ERROR VALUE\"]";
		}
	}
	else
	{
		result = "[0, \"ERROR UID\"]";
	}
}


void DB_BASIC_V2::getCharOption(Poco::Data::Session &db_session, std::string &steamid, std::string &option, std::string &result)
{
	if (isNumber(steamid))
	{
		Poco::Data::Statement sql(db_session);
		sql << ("SELECT `" + option + "` FROM `Player Info` WHERE SteamID=" + steamid), Poco::Data::into(result), Poco::Data::now;
		result = "[1, [" + result + "]]";
		if (!Sqf::check(result))
		{
			result = "[0, \"ERROR VALUE\"]";
		}
	}
	else
	{
		result = "[0, \"ERROR SteamID\"]";
	}
}


void DB_BASIC::setOption(Poco::Data::Session &db_session, std::string &table, std::string &uid, std::string &option, std::string value, std::string &result)
{
	if (isNumber(uid))
	{
		//if (Sqf::check(value))
		if (true)
		{
			Poco::Data::Statement sql(db_session);
			sql << ("UPDATE \"" + table + "\" SET `" + option + "` = '" + value + "' WHERE UID=" + uid), Poco::Data::now;
			result = "[1]";
		}
		else
		{
			result = "[0, \"ERROR VALUE\"]";
		}
	}
	else
	{
		result = "[0, \"ERROR UID\"]";
	}
}

void DB_BASIC::setCharOption(Poco::Data::Session &db_session, std::string &steamid, std::string &option, std::string value, std::string &result)
{
	if (isNumber(steamid))
	{
		//if (Sqf::check(value))
		if (true)
		{
			Poco::Data::Statement sql(db_session);
			sql << ("UPDATE \"Player Info\" SET `" + option + "` = '" + value + "' WHERE SteamID=" + steamid), Poco::Data::now;
			result = "[1]";
		}
		else
		{
			result = "[0, \"ERROR VALUE\"]";
		}
	}
	else
	{
		result = "[0, \"ERROR SteamID\"]";
	}
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
Other2				7
Other3				8
Everything alive	9

Get					5
Save		0-		2
*/

//setValue(table, uid, type, value)
//getValue(table, uid, type, value)

void DB_BASIC::callProtocol(AbstractExt *extension, std::string input_str, std::string &result)
{
	try
	{
		#ifdef TESTING
			std::cout << "extDB: DB_BASIC: DEBUG INFO: " + input_str << std::endl;
		#endif
		#ifdef DEBUG_LOGGING
			BOOST_LOG_SEV(logger, boost::log::trivial::trace) << "extDB: DB_BASIC: Trace: Input:" + input_str;
		#endif
		if (input_str.length() <= 4)
		{
			result = "[0,\"Error Message to Short\"]";
			#ifdef TESTING
				std::cout << "extDB: DB_BASIC: Error Message to Short: Input: " + input_str << std::endl;
			#endif
			BOOST_LOG_SEV(logger, boost::log::trivial::warning) << "extDB: DB_BASIC: Error Message to Short: Input: " + input_str;
		}
		else
		{
			bool option_all = false;
			bool option_other = false;
			std::string option;
			std::string value;
			
			const std::string sep_char(":");
			const std::string::size_type found = input_str.find(sep_char,4);
			
			if (found==std::string::npos)
			{
				result = "[0,\"Error Invalid Format\"]";
				#ifdef TESTING
					std::cout << "extDB: DB_BASIC: Error Invalid Format: Input: " + input_str << std::endl;
				#endif
				BOOST_LOG_SEV(logger, boost::log::trivial::warning) << "extDB: DB_BASIC: Error Invalid Format: Input: " + input_str;
			}
			else
			{
				std::string uid = input_str.substr(4,found-4);
				std::string value = input_str.substr(found+1);
				
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
						option = "Other 1";
						option_other = true;
						break;
					case 7:
						option = "Other 2";
						option_other = true;
						break;
					case 8:
						option = "Other 3";
						option_other = true;
						break;
					default:
						option = "Model";
				}
				
				switch (Poco::NumberParser::parse(input_str.substr(1,1)))
				{
					case (0):  // Player Info
					{
						if (option_other)
						{
							std::string table = "Player Info";
							if ((Poco::NumberParser::parse(input_str.substr(0,1))) == 5)
							{
								getCharOption(db_session, value, option, result);
							}
							else
							{
								setCharOption(db_session, uid, option, value, result);
							}
						}
						else
						{
							getCharUID(db_session, value, result);
						}
						break;
					}
					case (1):  // Player Char  "Player Characters"
					{
						std::string table = "Player Characters";
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
					{
						result = "[0,\"Error Unknown Option\"]";
						#ifdef TESTING
							std::cout << "extDB: DB_BASIC: Error Unknown Option: Input: " + input_str << std::endl;
						#endif
						BOOST_LOG_SEV(logger, boost::log::trivial::warning) << "extDB: DB_BASIC: Error Unknown Option: Input: " + input_str;
					}
				}
				#ifdef TESTING
					std::cout << "extDB: DB_BASIC: Trace: Result:" + result << std::endl;
				#endif
				#ifdef DEBUG_LOGGING
					BOOST_LOG_SEV(logger, boost::log::trivial::trace) << "extDB: DB_BASIC: Trace: Result: " + input_str;
				#endif
			}
		}
	}
	catch (Poco::Data::SQLite::DBLockedException& e)
	{
		#ifdef TESTING
			std::cout << "extDB: DB_BASIC: Error Database Locked Exception: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(logger, boost::log::trivial::warning) << "extDB: DB_BASIC: Error Database Locked Exception: " + e.displayText();
		BOOST_LOG_SEV(logger, boost::log::trivial::warning) << "eextDB: DB_BASIC: Error Database Locked Exception: Input:" + input_str;
		result = "[0,\"Error DB Locked Exception\"]";
	}
	catch (Poco::Data::DataException& e)
    {
		#ifdef TESTING
			std::cout << "extDB: DB_BASIC: Error Data Exception: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(logger, boost::log::trivial::warning) << "extDB: DB_BASIC: Error Data Exception: " + e.displayText();
		BOOST_LOG_SEV(logger, boost::log::trivial::warning) << "eextDB: DB_BASIC: Error Data Exception: Input:" + input_str;
        result = "[0,\"Error Data Exception\"]";
    }
    catch (Poco::Exception& e)
	{
		#ifdef TESTING
			std::cout << "extDB: DB_BASIC: Error Exception: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(logger, boost::log::trivial::warning) << "extDB: DB_BASIC: Error Exception: " + e.displayText();
		BOOST_LOG_SEV(logger, boost::log::trivial::warning) << "extDB: DB_BASIC: Error Exception: Input:" + input_str;
		result = "[0,\"Error Exception\"]";
	}
}
