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


#include "db_custom_v2.h"

#include <Poco/Data/Common.h>
#include <Poco/Data/MetaColumn.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>

#include "Poco/Data/MySQL/Connector.h"
#include "Poco/Data/MySQL/MySQLException.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Data/SQLite/SQLiteException.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Data/SQLite/SQLiteException.h"
#include "Poco/Data/ODBC/Connector.h"
#include "Poco/Data/ODBC/ODBCException.h"

#include <Poco/Exception.h>

#include <Poco/File.h>
#include <Poco/Path.h>
#include <Poco/String.h>

#include <Poco/StringTokenizer.h>
#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/Util/IniFileConfiguration.h>


#include <cstdlib>
#include <iostream>

#include "../sanitize.h"


bool DB_CUSTOM_V2::init(AbstractExt *extension, const std::string init_str)
{
	pLogger = &Poco::Logger::get(("DB_CUSTOM_V2:" + init_str));
	
	bool status = false;
	
	if (extension->getDBType() == std::string("MySQL"))
	{
		status = true;
	}
	else if (extension->getDBType() == std::string("ODBC"))
	{
		status =  true;
	}
	else if (extension->getDBType() == std::string("SQLite"))
	{
		status =  true;
	}
	else
	{
		// DATABASE NOT SETUP YET
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V2: No Database Connection" << std::endl;
		#endif
		pLogger->warning("No Database Connection");
		return false;
	}
	
	Poco::DateTime now;
	Poco::Path template_path;
	template_path.pushDirectory("extDB");
	template_path.pushDirectory("templates");
	Poco::File(template_path).createDirectories();
	template_path.setFileName(init_str + ".ini");
	template_ini = (new Poco::Util::IniFileConfiguration(template_path.toString()));
	
	//std::vector < std::string > calls = template_ini->createView("");
	std::vector < std::string > custom_calls;
	template_ini->keys(custom_calls);
	
	for(std::vector<std::string>::iterator it = custom_calls.begin(); it != custom_calls.end(); ++it) 
	{
		std::string call_name = *it;
		std::string sql;
		
		int sql_count = 1;
		std::string sql_count_str = "1";
		while (template_ini->has(call_name + ".SQL" + sql_count_str))
		{
			sql = sql + template_ini->getString(call_name + ".SQL1");
			sql_count++;
			sql_count_str = Poco::NumberFormatter::format(sql_count);
		}
		custom_protocol[call_name].sql = sql;
		custom_protocol[call_name].number_of_inputs = template_ini->getInt(call_name + ".Number of Inputs");
		custom_protocol[call_name].sanitize_inputs = template_ini->getBool(call_name + ".Sanitize Input");
		custom_protocol[call_name].sanitize_outputs = template_ini->getBool(call_name + ".Sanitize Output");
	}
	
	return status;
}

void DB_CUSTOM_V2::callCustomProtocol(AbstractExt *extension, boost::unordered_map<std::string, Template_Calls>::const_iterator itr, Poco::StringTokenizer &tokens, int &token_count, std::string &result)
{
	std::string input_str(itr->second.sql);
	for (int x=0; x < token_count; x++)
	{
		std::string input_variable = "$INPUT_" + Poco::NumberFormatter::format(x);
		Poco::replaceInPlace(input_str, input_variable, tokens[(x+1)]);
	}
		
	try 
	{
		Poco::Data::Session db_session = extension->getDBSession_mutexlock();
		Poco::Data::Statement sql(db_session);
		sql << input_str;
		sql.execute();
		Poco::Data::RecordSet rs(sql);

		result = "[1, [";
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
		result += "]]";
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V2: DEBUG INFO: RESULT:" + result << std::endl;
		#endif
		#ifdef DEBUG_LOGGING
			pLogger->trace(" RESULT:" + result);
		#endif
	}
	catch (Poco::Data::SQLite::DBLockedException& e)
	{
		#ifdef TESTING
			std::cout << "extDB: Error: " << e.displayText() << std::endl;
		#endif 
		pLogger->critical("Input: " + input_str);
		pLogger->critical("Database Locked Exception: " + e.displayText());
		result = "[0,\"Error DBLocked Exception\"]";
	}
	catch (Poco::Data::MySQL::ConnectionException& e)
	{
		#ifdef TESTING
			std::cout << "extDB: Error: " << e.displayText() << std::endl;
		#endif 
		pLogger->critical("Input: " + input_str);
		pLogger->critical("Connection Exception: " + e.displayText());
		result = "[0,\"Error Connection Exception\"]";
	}
	catch(Poco::Data::MySQL::StatementException& e)
	{
		#ifdef TESTING
			std::cout << "extDB: Error: " << e.displayText() << std::endl;
		#endif 
		pLogger->critical("Input: " + input_str);
		pLogger->critical("Statement Exception: " + e.displayText());
		result = "[0,\"Error Statement Exception\"]";
	}
	catch (Poco::Data::DataException& e)
    {
		#ifdef TESTING
			std::cout << "extDB: Error: " << e.displayText() << std::endl;
		#endif
		pLogger->critical("Input: " + input_str);
		pLogger->critical("Data Exception: " + e.displayText());
        result = "[0,\"Error Data Exception\"]";
    }
    catch (Poco::Exception& e)
	{
		#ifdef TESTING
			std::cout << "extDB: Error: " << e.displayText() << std::endl;
		#endif
		pLogger->critical("Input: " + input_str);
		pLogger->critical("Exception: " + e.displayText());
		result = "[0,\"Error Exception\"]";
	}
}


void DB_CUSTOM_V2::callProtocol(AbstractExt *extension, std::string input_str, std::string &result)
{
	Poco::StringTokenizer tokens(input_str, ":");
	
	int token_count = tokens.count();
	if (token_count == 0)
	{
		result = "[0,\"Error WTF\"]"; // TODO:: Go through code + check if this error is possible
	}
	else
	{
		boost::unordered_map<std::string, Template_Calls>::const_iterator itr = custom_protocol.find(tokens[0]);
		if (itr == custom_protocol.end())
		{
			result = "[0,\"Error No Custom Call Not Found\"]";
		}
		else
		{
			if (itr->second.number_of_inputs != (token_count - 1))
			{
				result = "[0,\"Error Incorrect Number of Inputs\"]";
			}
			else
			{
				bool sanitize_check = true;
				if (itr->second.sanitize_inputs)
				{
					for(int i = 0; i != token_count; ++i) {
						if (!Sqf::check(tokens[i]))
						{
							sanitize_check = false;
							break;
						}
					}
				}
				if (sanitize_check)
				{
					callCustomProtocol(extension, itr, tokens, token_count, result);
				}
			}
		}
	}
	
	
	#ifdef TESTING
		std::cout << "extDB: DB_CUSTOM_V2: DEBUG INFO: " + input_str << std::endl;
	#endif
	#ifdef DEBUG_LOGGING
		pLogger->trace(" " + input_str);
	#endif
}