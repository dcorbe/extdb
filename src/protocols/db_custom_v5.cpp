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

#include "db_custom_v5.h"

#include <Poco/Data/Common.h>
#include <Poco/Data/MetaColumn.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>

#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Data/SQLite/Connector.h>
#include <Poco/Data/SQLite/SQLiteException.h>

#include <Poco/DynamicAny.h>
#include <Poco/Exception.h>
#include <Poco/String.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Util/AbstractConfiguration.h>
#include <Poco/Util/IniFileConfiguration.h>

#include <Poco/DigestEngine.h>
#include <Poco/MD5Engine.h>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>

#include <algorithm>


#ifdef TESTING
	#include <iostream>
#endif

#include "../sanitize.h"


bool DB_CUSTOM_V5::init(AbstractExt *extension, const std::string init_str)
{
	db_custom_name = init_str;
	
	bool status = false;
	
	if (extension->getDBType() == std::string("MySQL"))
	{
		status = true;
	}
	else if (extension->getDBType() == std::string("SQLite"))
	{
		status =  true;
	}
	else
	{
		// DATABASE NOT SETUP YET
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V5: No Database Connection" << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: No Database Connection";
		return false;
	}

	// Check if DB_CUSTOM_V5 Template Filename Given
	if (init_str.empty()) 
	{
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V5: Missing Parameter or No Template Filename given" << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "Missing Parameter or No Template Filename given";
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

		if (template_ini->hasOption("Default.Version"))
		{
			bool default_sanitize_value_check = template_ini->getBool("Default.Sanitize Value Check", true);
			bool default_string_datatype_check = template_ini->getBool("Default.String Datatype Check", true);

			std::string default_bad_chars = template_ini->getString("Default.Bad Chars");
			std::string default_bad_chars_action = template_ini->getString("Default.Bad Chars Action", "STRIP");

			if ((template_ini->getInt("Default.Version", 1)) == 5)
			{
				for(std::vector<std::string>::iterator it = custom_calls.begin(); it != custom_calls.end(); ++it) 
				{
					int sql_line_num = 0;
					int sql_part_num = 0;
					std::string call_name = *it;

					std::string sql_line_num_str;
					std::string sql_part_num_str;

					custom_protocol[call_name].string_datatype_check = template_ini->getBool(call_name + ".String Datatype Check", default_string_datatype_check);
					custom_protocol[call_name].sanitize_value_check = template_ini->getBool(call_name + ".Sanitize Value Check", default_sanitize_value_check);
					custom_protocol[call_name].bad_chars_action = template_ini->getString(call_name + ".Bad Chars Action", default_bad_chars_action);
					custom_protocol[call_name].bad_chars = template_ini->getString(call_name + ".Bad Chars", default_bad_chars);

					while (true)
					{
						if ((boost::iequals(custom_protocol[call_name].bad_chars_action, std::string("NONE")) == 1) || 
							(boost::iequals(custom_protocol[call_name].bad_chars_action, std::string("STRIP")) == 1)  ||
							(boost::iequals(custom_protocol[call_name].bad_chars_action, std::string("STRIP+LOG")) == 1)  ||
							(boost::iequals(custom_protocol[call_name].bad_chars_action, std::string("STRIP+ERROR")) == 1)) 
						{
							// Prepared Statement
							sql_line_num++;
							sql_line_num_str = Poco::NumberFormatter::format(sql_line_num);
							if (!(template_ini->has(call_name + ".SQL" + sql_line_num_str + "_1")))
							{
								break;
							}
							
							std::string sql_str;

							sql_part_num = 0;
							while (true)
							{
								sql_part_num++;
								sql_part_num_str = Poco::NumberFormatter::format(sql_part_num);							
								if (!(template_ini->has(call_name + ".SQL" + sql_line_num_str + "_" + sql_part_num_str)))
								{
									break;
								}
								else
								{
									sql_str += (template_ini->getString(call_name + ".SQL" + sql_line_num_str + "_" + sql_part_num_str)) + " " ;		
								}
							}

							if (sql_part_num > 1) // Remove trailing Whitespace
							{
								sql_str = sql_str.substr(0, sql_str.size()-1);
							}

							custom_protocol[call_name].sql_prepared_statements.push_back(sql_str);


							// Inputs
							Poco::StringTokenizer tokens_input((template_ini->getString(call_name + ".INPUTS", "")), ",");
							for (int x = 0; x < (tokens_input.count()); x++)
							{
								// 0 Nothing
								// 1 Add Quotes
								// 2 Convert to BEGUID

								if ((boost::iequals(tokens_input[x], std::string("N")) == 1) || (boost::iequals(tokens_input[x], std::string("Nothing")) == 1))
								{
									custom_protocol[call_name].sql_inputs.push_back(0);
								}
								else if ((boost::iequals(tokens_input[x], std::string("S")) == 1) || (boost::iequals(tokens_input[x], std::string("String")) == 1))
								{
									custom_protocol[call_name].sql_inputs.push_back(1);
								}
								else if ((boost::iequals(tokens_input[x], std::string("B")) == 1) || (boost::iequals(tokens_input[x], std::string("BEGUID")) == 1))
								{
									custom_protocol[call_name].sql_inputs.push_back(2);
								}
								else
								{
									status = false;
									#ifdef TESTING
										std::cout << "extDB: DB_CUSTOM_V5: Bad Output Option: " << call_name << ":" << tokens_input[x] << std::endl;
									#endif
									BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V5: Bad Output Option " << call_name << ":" << tokens_input[x];
								}
							}



							// Outputs
							Poco::StringTokenizer tokens_output((template_ini->getString(call_name + ".OUTPUT", "")), ",");

							for (int x = 0; x < (tokens_output.count()); x++)
							{
								// 0 Nothing
								// 1 Add Quotes
								// 9 Auto String Check

								if ((boost::iequals(tokens_output[x], std::string("N")) == 1) || (boost::iequals(tokens_output[x], std::string("Nothing")) == 1))
								{
									custom_protocol[call_name].sql_outputs.push_back(0);
								}
								else if ((boost::iequals(tokens_output[x], std::string("S")) == 1) || (boost::iequals(tokens_output[x], std::string("String")) == 1))
								{
									custom_protocol[call_name].sql_outputs.push_back(1);
								}
								else if ((boost::iequals(tokens_output[x], std::string("A")) == 1) || (boost::iequals(tokens_output[x], std::string("Auto")) == 1))
								{
									custom_protocol[call_name].sql_outputs.push_back(9);
								}
								else
								{
									status = false;
									#ifdef TESTING
									std::cout << "extDB: DB_CUSTOM_V5: Bad Output Option: " << call_name << ":" << tokens_output[x] << std::endl;
									#endif
									BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V5: Bad Output Option " << call_name << ":" << tokens_output[x];
								}
							}
						}
						else
						{
							status = false;
							#ifdef TESTING
								std::cout << "extDB: DB_CUSTOM_V5: Unknown Bad Strings Action for " << call_name << ":" << custom_protocol[call_name].bad_chars_action << std::endl;
							#endif
							BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V5: Unknown Bad Strings Action for " << call_name << ":" << custom_protocol[call_name].bad_chars_action;
						}
					}
				}
			}
			else
			{
				status = false;
				#ifdef TESTING
					std::cout << "extDB: DB_CUSTOM_V5: Template File Missing Incompatiable Version" << db_template_file << std::endl;
				#endif
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V5: Template File Missing Incompatiable Version: " << db_template_file;
			}
		}
		else
		{
			status = false;
			#ifdef TESTING
				std::cout << "extDB: DB_CUSTOM_V5: Template File Missing Default Options" << db_template_file << std::endl;
			#endif
			BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V5: Template File Missing Default Options: " << db_template_file;
		}
	}
	else 
	{
		status = false;
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V5: Template File Not Found" << db_template_file << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V5: No Template File Found: " << db_template_file;
	}
	return status;
}


void DB_CUSTOM_V5::getBEGUID(std::string &input_str, std::string &result)
// From Frank https://gist.github.com/Fank/11127158
// Modified to use libpoco
{
	bool status = true;
	
	if (input_str.empty())
	{
		status = false;
		result = "Invalid SteamID";
	}
	else
	{
		for (unsigned int index=0; index < input_str.length(); index++)
		{
			if (!std::isdigit(input_str[index]))
			{
				status = false;
				result = "Invalid SteamID";
				break;
			}
		}
	}
	if (status)
	{
		Poco::Int64 steamID = Poco::NumberParser::parse64(input_str);

		Poco::Int8 i = 0;
		Poco::Int8 parts[8] = { 0 };

		do
		{
			parts[i++] = steamID & 0xFFu;
		} while (steamID >>= 8);

		std::stringstream bestring;
		bestring << "BE";
		for (int i = 0; i < sizeof(parts); i++) {
			bestring << char(parts[i]);
		}

		boost::lock_guard<boost::mutex> lock(mutex_md5);
		md5.update(bestring.str());
		result = Poco::DigestEngine::digestToHex(md5.digest());
	}
}


void DB_CUSTOM_V5::getResult(std::unordered_map<std::string, Template_Calls>::const_iterator itr, Poco::Data::Statement &sql_statement, std::string &result)
{
	Poco::Data::RecordSet rs(sql_statement);

	result = "[1,[";
	std::size_t cols = rs.columnCount();
	if (cols >= 1)
	{
		bool more = rs.moveFirst();
		while (more)
		{
			result += "[";
			std::size_t sql_output_count = itr->second.sql_outputs.size();

			for (std::size_t col = 0; col < cols; ++col)
			{
				std::string temp_str = rs[col].convert<std::string>();
				
				if (col >= sql_output_count)
				{
					if (itr->second.string_datatype_check)
					{
						if (rs.columnType(col) == Poco::Data::MetaColumn::FDT_STRING)
						{
							if (temp_str.empty())
							{
								result += ("\"\"");
							}
							else
							{
								result += "\"" + temp_str + "\"";
							}
						}
						else
						{
							if (temp_str.empty())
							{
								result += ("\"\"");
							}
							else
							{
								result += temp_str;
							}
						}
					}
					else
					{
						result += temp_str;
					}
				}
				else
				{
					switch (itr->second.sql_outputs[col])
					{
						case 1: // S String
							if (temp_str.empty())
							{
								result += ("\"\"");
							}
							else
							{
								result += "\"" + temp_str + "\"";
							}
							break;

						case 2: // A Auto
							if (rs.columnType(col) == Poco::Data::MetaColumn::FDT_STRING)
							{
								if (temp_str.empty())
								{
									result += ("\"\"");
								}
								else
								{
									result += "\"" + temp_str + "\"";
								}
							}
							else
							{
								if (temp_str.empty())
								{
									result += ("\"\"");
								}
								else
								{
									result += temp_str;
								}
							}
							break;

						default: // 0 ==  N Nothing 
							result += temp_str;
					}
				}

				if (col < (cols - 1))
				{
					result += ",";
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
}


void DB_CUSTOM_V5::executeSQL(AbstractExt *extension, Poco::Data::Statement &sql_statement, std::string &result, bool &status)
{
	try
	{
		sql_statement.execute();
	}

	catch (Poco::Data::SQLite::DBLockedException& e)
	{
		status = false;
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V4: Error DBLockedException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error DBLockedException: " + e.displayText();
		result = "[0,\"Error DBLocked Exception\"]";
	}
	catch (Poco::Data::MySQL::ConnectionException& e)
	{
		status = false;
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V4: Error ConnectionException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error ConnectionException: " + e.displayText();
	}
	catch(Poco::Data::MySQL::StatementException& e)
	{
		status = false;
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V4: Error StatementException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error StatementException: " + e.displayText();
		result = "[0,\"Error Statement Exception\"]";
	}
	catch (Poco::Data::DataException& e)
	{
		status = false;
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V4: Error DataException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error DataException: " + e.displayText();
		result = "[0,\"Error Data Exception\"]";
	}
	catch (Poco::Exception& e)
	{
		status = false;
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V4: Error Exception: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error Exception: " + e.displayText();
		result = "[0,\"Error Exception\"]";
	}
}


void DB_CUSTOM_V5::callCustomProtocol(AbstractExt *extension, std::string call_name, std::unordered_map<std::string, Template_Calls>::const_iterator itr, std::vector< std::string > &tokens, std::string &input_str, bool &sanitize_value_check_ok, std::string &result)
{
	// TODO Sanitize Value Check

	if (sanitize_value_check_ok)
	{
		bool status = true;
		std::tuple<Poco::Data::Session, Poco::Data::SessionPool::StatementCacheMap, Poco::Data::SessionPool::SessionList::iterator> db_customSession = extension->getDBSessionCustom_mutexlock();

		Poco::Data::SessionPool::StatementCacheMap::iterator statement_cache_itr = std::get<1>(db_customSession).find(call_name);
		if (statement_cache_itr == std::get<1>(db_customSession).end())
		{
			// NO CACHE
			int i = 0;
			for (std::vector< std::string >::const_iterator it_sql_prepared_statements_vector = itr->second.sql_prepared_statements.begin(); it_sql_prepared_statements_vector != itr->second.sql_prepared_statements.end(); ++it_sql_prepared_statements_vector)
			{
				i++;

				std::get<1>(db_customSession)[call_name].inputs[i].insert(std::get<1>(db_customSession)[call_name].inputs[i].begin(), tokens.begin(), tokens.end());

				Poco::Data::Statement sql_statement(std::get<0>(db_customSession));
				std::get<1>(db_customSession)[call_name].statements.push_back(sql_statement);
				sql_statement << *it_sql_prepared_statements_vector, Poco::Data::use(std::get<1>(db_customSession)[call_name].inputs[i]);
				executeSQL(extension, sql_statement, result, status);

				std::get<1>(db_customSession)[call_name].inputs[i].clear();

				if ( it_sql_prepared_statements_vector+1 == itr->second.sql_prepared_statements.end() )
				{
					getResult(itr, sql_statement, result);
				}
			}
		}
		else
		{
			// CACHE
			for (std::vector<int>::size_type i = 0; i != std::get<1>(db_customSession)[call_name].statements.size(); i++)
			{
				statement_cache_itr->second.inputs[i].insert(statement_cache_itr->second.inputs[i].begin(), tokens.begin(), tokens.end());

				executeSQL(extension, statement_cache_itr->second.statements[i], result, status);
				statement_cache_itr->second.inputs[i].clear();

				if (i == (std::get<1>(db_customSession)[call_name].statements.size() - 1))
				{
					getResult(itr, statement_cache_itr->second.statements[i], result);
				}
			}
		}
		extension->putbackDBSessionPtr_mutexlock(std::get<2>(db_customSession));

		if (!status)
		{
			BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error Exception: SQL:" + input_str;
		}
		else
		{
			#ifdef TESTING
				std::cout << "extDB: DB_CUSTOM_V5: Trace: Result: " + result << std::endl;
			#endif
			#ifdef DEBUG_LOGGING
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::trace) << "extDB: DB_CUSTOM_V5: Trace: Result: " + result;
			#endif
		}
	}

	//TODO IF Exception Encountered REMOVE CACHED PREPARED STATEMENT, reduces code complexity
}


void DB_CUSTOM_V5::callProtocol(AbstractExt *extension, std::string input_str, std::string &result)
{
	#ifdef TESTING
		std::cout << "extDB: DB_CUSTOM_V5: Trace: " + input_str << std::endl;
	#endif
	#ifdef DEBUG_LOGGING
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::trace) << "extDB: DB_CUSTOM_V5: Trace: Input:" + input_str;
	#endif

	Poco::StringTokenizer tokens(input_str, ":");
	
	int token_count = tokens.count();
	std::unordered_map<std::string, Template_Calls>::const_iterator itr = custom_protocol.find(tokens[0]);
	if (itr == custom_protocol.end())
	{
		result = "[0,\"Error No Custom Call Not Found\"]";
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: Error No Custom Call Not Found: " + input_str;
	}
	else
	{
		if (itr->second.number_of_inputs != (token_count - 1))
		{
			result = "[0,\"Error Incorrect Number of Inputs\"]";
			BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: Incorrect Number of Inputs: " + input_str;
		}
		else
		{
			bool sanitize_value_check_ok = true;

			std::vector< std::string > inputs;
			std::string input_value_str;

			bool bad_chars_error = false;
			if (boost::iequals(itr->second.bad_chars_action, std::string("STRIP")) == 1)
			{
				for(int i = 0; i < token_count; ++i) 
				{
					input_value_str = tokens[i];

					// Strip Chars					
					for (int i2 = 0; (i2 < (itr->second.bad_chars.size() - 1)); ++i2)
					{
						boost::erase_all(input_value_str, std::string(1,itr->second.bad_chars[i2]));
					}

					// Convert Input
					switch (itr->second.sql_inputs[i])
						// 0 Nothing
						// 1 Add Quotes
						// 2 Convert to BEGUID
					{
						case 1:
						{					
							boost::erase_all(input_value_str, "\"");
							input_value_str = "\"" + input_value_str + "\"";

							if (itr->second.sanitize_value_check)
							{
								if (!Sqf::check(input_value_str))
								{
									sanitize_value_check_ok = false;
								}
							}
							break;
						}
						case 2:
						{
							getBEGUID(input_value_str, input_value_str);

							if (itr->second.sanitize_value_check)
							{
								if (!Sqf::check(input_value_str))
								{
									sanitize_value_check_ok = false;
								}
							}
							break;
						}
					}
					// Add String to List
					inputs.push_back(input_value_str);
				}
			}
			else if (boost::iequals(itr->second.bad_chars_action, std::string("STRIP+LOG")) == 1)
			{
				for(int i = 0; i < token_count; i++)
				{
					input_value_str = tokens[i];

					// Strip Chars					
					for (int i2 = 0; (i2 < (itr->second.bad_chars.size() - 1)); ++i2)
					{
						boost::erase_all(input_value_str, std::string(1,itr->second.bad_chars[i2]));
					}

					if (input_value_str != tokens[i])
					{
						BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: Error Bad Char Detected: Input:" + input_str;
						BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: Error Bad Char Detected: Token:" + tokens[i];
					}

					// Convert Input
					switch (itr->second.sql_inputs[i])
						// 0 Nothing
						// 1 Add Quotes
						// 2 Convert to BEGUID
					{
						case 1:
						{
							boost::erase_all(input_value_str, "\"");
							input_value_str = "\"" + input_value_str + "\"";

							if (itr->second.sanitize_value_check)
							{
								if (!Sqf::check(input_value_str))
								{
									sanitize_value_check_ok = false;
								}
							}
							break;
						}
						case 2:
						{
							getBEGUID(input_value_str, input_value_str);

							if (itr->second.sanitize_value_check)
							{
								if (!Sqf::check(input_value_str))
								{
									sanitize_value_check_ok = false;
								}
							}
							break;
						}
					}
					// Add String to List
					inputs.push_back(input_value_str);
				}
			}
			else if (boost::iequals(itr->second.bad_chars_action, std::string("STRIP+ERROR")) == 1)
			{
				for(int i = 0; i < token_count; i++)
				{
					input_value_str = tokens[i];

					// Strip Chars					
					for (int i2 = 0; (i2 < (itr->second.bad_chars.size() - 1)); ++i2)
					{
						boost::erase_all(input_value_str, std::string(1,itr->second.bad_chars[i2]));
					}

					if (input_value_str != tokens[i])
					{
						BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: Error Bad Char Detected: Input:" + input_str;
						BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: Error Bad Char Detected: Token:" + tokens[i];
						bad_chars_error = true;
					}

					// Convert Input
					switch (itr->second.sql_inputs[i])
						// 0 Nothing
						// 1 Add Quotes
						// 2 Convert to BEGUID
					{
						case 1:
						{
							boost::erase_all(input_value_str, "\"");
							input_value_str = "\"" + input_value_str + "\"";

							if (itr->second.sanitize_value_check)
							{
								if (!Sqf::check(input_value_str))
								{
									sanitize_value_check_ok = false;
								}
							}
							break;
						}
						case 2:
						{
							getBEGUID(input_value_str, input_value_str);

							if (itr->second.sanitize_value_check)
							{
								if (!Sqf::check(input_value_str))
								{
									sanitize_value_check_ok = false;
								}
							}
							break;
						}
					}
					// Add String to List
					inputs.push_back(input_value_str);
				}
			}
			else
			{
				inputs.insert( inputs.end(), tokens.begin(), tokens.end());
			}

			if (!(bad_chars_error))
			{
				if (!sanitize_value_check_ok)
				{
					result = "[0,\"Error Values Input is not sanitized\"]";
					BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: Sanitize Check error: Input:" + input_str;
				}
				else
				{
					callCustomProtocol(extension, tokens[0], itr, inputs, input_str, sanitize_value_check_ok, result);
				}
			}
			else
			{
				result = "[0,\"Error Bad Char Found\"]";
			}
		}
	}
}