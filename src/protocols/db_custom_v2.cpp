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

#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Data/SQLite/Connector.h>
#include <Poco/Data/SQLite/SQLiteException.h>
#include <Poco/Data/ODBC/Connector.h>
#include <Poco/Data/ODBC/ODBCException.h>

#include <Poco/DynamicAny.h>
#include <Poco/Exception.h>
#include <Poco/String.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/Util/IniFileConfiguration.h>

#include <boost/filesystem.hpp>

#ifdef TESTING
	#include <iostream>
#endif

#include "../sanitize.h"


bool DB_CUSTOM_V2::init(AbstractExt *extension, const std::string init_str)
{
	db_custom_name = init_str;
	
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
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V2: No Database Connection";
		return false;
	}
	
	std::string db_custom_dir = boost::filesystem::path("extDB/db_custom").make_preferred().string();
	boost::filesystem::create_directories(db_custom_dir); // Creating Directory if missing
	std::string db_template_file = boost::filesystem::path(db_custom_dir + "/" + init_str + ".ini").make_preferred().string();

	
	if (boost::filesystem::exists(db_template_file))
	{
		template_ini = (new Poco::Util::IniFileConfiguration(db_template_file));
		
		std::vector < std::string > custom_calls;
		template_ini->keys(custom_calls);
		
		for(std::vector<std::string>::iterator it = custom_calls.begin(); it != custom_calls.end(); ++it) 
		{
			std::string call_name = *it;
			std::string sql_str;
			
			int sql_count = 1;
			std::string sql_count_str = "1";
			while (template_ini->has(call_name + ".SQL_" + sql_count_str))
			{
				sql_str += template_ini->getString(call_name + ".SQL_" + sql_count_str);
				sql_count++;
				sql_count_str = Poco::NumberFormatter::format(sql_count);
			}
			custom_protocol[call_name].number_of_inputs = template_ini->getInt(call_name + ".Number of Inputs", 0);
			custom_protocol[call_name].sanitize_inputs = template_ini->getBool(call_name + ".Sanitize Input", true);
			custom_protocol[call_name].sanitize_outputs = template_ini->getBool(call_name + ".Sanitize Output", true);
			
			std::list<Poco::DynamicAny> sql_list;
			sql_list.push_back(Poco::DynamicAny(sql_str));

			for (int x=1; x <= custom_protocol[call_name].number_of_inputs; ++x)
			{
				std::string input_val_str = "$INPUT_" + Poco::NumberFormatter::format(x);
				size_t input_val_len = input_val_str.length();

				for(std::list<Poco::DynamicAny>::iterator it_sql_list = sql_list.begin(); it_sql_list != sql_list.end(); ++it_sql_list) 
				{
					if (it_sql_list->isString())
					{
						size_t pos;
						std::string work_str = *it_sql_list;
						std::string tmp_str;
						while(true)
						{
							pos = work_str.find(input_val_str);
							if (pos != std::string::npos)
							{
								tmp_str = work_str.substr(0, pos);
								work_str = work_str.substr((pos + input_val_len), std::string::npos);
								
								std::list<Poco::DynamicAny>::iterator new_it_sql_vector = sql_list.insert(it_sql_list, x);
								new_it_sql_vector = sql_list.insert(new_it_sql_vector, tmp_str);
							}
							else
							{
								if (!work_str.empty())
								{
									sql_list.insert(it_sql_list, work_str);
									it_sql_list = sql_list.erase(it_sql_list);
									--it_sql_list;
								}
								break;
							}
						}
					}
				}
			}
			custom_protocol[call_name].sql = sql_list;
		}
	} 
	else 
	{
		status = false;
	}
	return status;
}


void DB_CUSTOM_V2::callCustomProtocol(AbstractExt *extension, boost::unordered_map<std::string, Template_Calls>::const_iterator itr, Poco::StringTokenizer &tokens, std::string &result)
{
	std::string sql_str;
	
	for(std::list<Poco::DynamicAny>::const_iterator it_sql_list = (itr->second.sql).begin(); it_sql_list != (itr->second.sql).end(); ++it_sql_list) 
	{
		if (it_sql_list->isString())  // Check for Input Variable
		{
			sql_str += it_sql_list->convert<std::string>();
		}
		else
		{
			sql_str += tokens[*it_sql_list];
		}
	}

	try 
	{
		Poco::Data::Session db_session = extension->getDBSession_mutexlock();
		Poco::Data::Statement sql(db_session);
		sql << sql_str;
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
			std::cout << "extDB: DB_CUSTOM_V2: Trace: Result: " + result << std::endl;
		#endif
		#ifdef DEBUG_LOGGING
			BOOST_LOG_SEV(extension->logger, boost::log::trivial::trace) << "extDB: DB_CUSTOM_V2: Trace: Result: " + result;
		#endif
	}
	catch (Poco::Data::SQLite::DBLockedException& e)
	{
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V2: Error DBLockedException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V2: Error DBLockedException: " + e.displayText();
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V2: Error DBLockedException: SQL:" + sql_str;
		result = "[0,\"Error DBLocked Exception\"]";
	}
	catch (Poco::Data::MySQL::ConnectionException& e)
	{
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V2: Error ConnectionException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V2: Error ConnectionException: " + e.displayText();
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V2: Error ConnectionException: SQL:" + sql_str;
	}
	catch(Poco::Data::MySQL::StatementException& e)
	{
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V2: Error StatementException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V2: Error StatementException: " + e.displayText();
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V2: Error StatementException: SQL:" + sql_str;
		result = "[0,\"Error Statement Exception\"]";
	}
	catch (Poco::Data::DataException& e)
    {
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V2: Error DataException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V2: Error DataException: " + e.displayText();
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V2: Error DataException: SQL:" + sql_str;
        result = "[0,\"Error Data Exception\"]";
    }
    catch (Poco::Exception& e)
	{
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V2: Error Exception: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V2: Error Exception: " + e.displayText();
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V2: Error Exception: SQL:" + sql_str;
		result = "[0,\"Error Exception\"]";
	}
}


void DB_CUSTOM_V2::callProtocol(AbstractExt *extension, std::string input_str, std::string &result)
{
	#ifdef TESTING
		std::cout << "extDB: DB_CUSTOM_V2: Trace: " + input_str << std::endl;
	#endif
	#ifdef DEBUG_LOGGING
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::trace) << "extDB: DB_CUSTOM_V2: Trace: Input:" + input_str;
	#endif

	Poco::StringTokenizer tokens(input_str, ":");
	
	int token_count = tokens.count();
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
				for(int i = 1; i < token_count; ++i) {
					if (!Sqf::check(tokens[i]))
					{
						std::cout << tokens[i] << std::endl;
						sanitize_check = false;
						result = "[0,\"Error Value Input is not sanitized\"]";
						break;
					}
				}
			}
			if (sanitize_check)
			{
				callCustomProtocol(extension, itr, tokens, result);
			}
		}
	}
}