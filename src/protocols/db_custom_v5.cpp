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

	
	// Read Template File
	if (boost::filesystem::exists(db_template_file))
	{
		template_ini = (new Poco::Util::IniFileConfiguration(db_template_file));
		
		std::vector < std::string > custom_calls;
		template_ini->keys(custom_calls);

		if (template_ini->hasOption("Default.Version"))
		{
			bool default_input_sanitize_value_check = template_ini->getBool("Default.Sanitize Input Value Check", true);
			bool default_output_sanitize_value_check = template_ini->getBool("Default.Sanitize Output Value Check", true);
			bool default_string_datatype_check = template_ini->getBool("Default.String Datatype Check", true);

			std::string default_bad_chars = template_ini->getString("Default.Bad Chars");
			int default_bad_chars_action = 0;

			std::string bad_chars_action_str = template_ini->getString("Default.Bad Chars Action", "STRIP");
			if	(boost::iequals(bad_chars_action_str, std::string("STRIP")) == 1)
			{
				default_bad_chars_action = 1;
			}
			else if	(boost::iequals(bad_chars_action_str, std::string("STRIP+LOG")) == 1)
			{
				default_bad_chars_action = 2;
			}
			else if	(boost::iequals(bad_chars_action_str, std::string("STRIP+ERROR")) == 1)
			{
				default_bad_chars_action = 3;
			}
			else if (boost::iequals(bad_chars_action_str, std::string("NONE")) == 1)
			{
				default_bad_chars_action = 0;
			}
			else
			{
				#ifdef TESTING
					std::cout << "extDB: DB_CUSTOM_V5: Invalid Default Bad Chars Action: " << bad_chars_action_str << std::endl;
				#endif
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: Invalid Default Bad Chars Action: " << bad_chars_action_str;
			}

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

					if (template_ini->has(call_name + ".Bad Chars Action"))
					{
						bad_chars_action_str = template_ini->getString(call_name + ".Bad Chars Action", "");
						if	(boost::iequals(bad_chars_action_str, std::string("STRIP")) == 1)
						{
							custom_protocol[call_name].bad_chars_action = 1;
						}
						else if	(boost::iequals(bad_chars_action_str, std::string("STRIP+LOG")) == 1)
						{
							custom_protocol[call_name].bad_chars_action = 2;
						}
						else if	(boost::iequals(bad_chars_action_str, std::string("STRIP+ERROR")) == 1)
						{
							custom_protocol[call_name].bad_chars_action = 3;
						}
						else if (boost::iequals(bad_chars_action_str, std::string("NONE")) == 1)
						{
							custom_protocol[call_name].bad_chars_action = 0;
						}
						else
						{
							#ifdef TESTING
								std::cout << "extDB: DB_CUSTOM_V5: " << call_name << ": Invalid Bad Chars Action: " << bad_chars_action_str << std::endl;
							#endif
							BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: " << call_name << ": Invalid Bad Chars Action: " << bad_chars_action_str;
						}
					}
					else
					{
						custom_protocol[call_name].bad_chars_action = default_bad_chars_action;
					}
					
					custom_protocol[call_name].bad_chars = template_ini->getString(call_name + ".Bad Chars", default_bad_chars);

					custom_protocol[call_name].input_sanitize_value_check = template_ini->getBool(call_name + ".Sanitize Value Check", default_input_sanitize_value_check);
					custom_protocol[call_name].output_sanitize_value_check = template_ini->getBool(call_name + ".Sanitize Value Check", default_output_sanitize_value_check);

					while (true)
					{
						if (custom_protocol[call_name].bad_chars_action > 0)
						{
							// Prepared Statement
							sql_line_num++;
							sql_line_num_str = Poco::NumberFormatter::format(sql_line_num);

							if (!(template_ini->has(call_name + ".SQL" + sql_line_num_str + "_1")))  // No More SQL Statements
							{
								// Initialize Default Output Options
								Value_Options outputs_options;
								outputs_options.check = default_output_sanitize_value_check;

								// Get Output Options
								Poco::StringTokenizer tokens_output_options((template_ini->getString(call_name + ".OUTPUT", "")), ",");

								for (int i = 0; i < (tokens_output_options.count()); i++)
								{
									if (!(Poco::NumberParser::tryParse(tokens_output_options[i], outputs_options.number)))
									{
										if (boost::iequals(tokens_output_options[i], std::string("STRING")) == 1)
										{
											custom_protocol[call_name].sql_outputs_options[i].string = true;
										}
										else if (boost::iequals(tokens_output_options[i], std::string("BEGUID")) == 1)
										{
											custom_protocol[call_name].sql_outputs_options[i].beguid = true;
										}
										else if (boost::iequals(tokens_output_options[i], std::string("CHECK")) == 1)
										{
											custom_protocol[call_name].sql_outputs_options[i].check = true;
										}
										else if (boost::iequals(tokens_output_options[i], std::string("NOCHECK")) == 1)
										{
											custom_protocol[call_name].sql_outputs_options[i].check = false;
										}
										else
										{
											status = false;
											#ifdef TESTING
												std::cout << "extDB: DB_CUSTOM_V5: Bad Output Option: " << call_name << ":" << tokens_output_options[i] << std::endl;
											#endif
											BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V5: Bad Output Option " << call_name << ":" << tokens_output_options[i];
										}
									}
								}
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


							// Get Input Options
							Poco::StringTokenizer tokens_input(template_ini->getString((call_name + ".SQL" + sql_line_num_str + "_INPUTS", "")), ",");

							// Initialize Default Input Options
							Value_Options inputs_options;
							inputs_options.check = default_input_sanitize_value_check;

							for (int x = 0; x < (tokens_input.count()); x++)
							{
								custom_protocol[call_name].sql_inputs_options[sql_line_num] = std::vector < Value_Options >();
								Poco::StringTokenizer tokens_input_options(tokens_input[x], "-");
								
								for (Poco::StringTokenizer::Iterator tokens_input_options_it = tokens_input_options.begin(); tokens_input_options_it != tokens_input_options.end(); ++tokens_input_options_it)
								{
									if (!(Poco::NumberParser::tryParse(*tokens_input_options_it, inputs_options.number)))
									{
										if (boost::iequals(*tokens_input_options_it, std::string("STRING")) == 1)
										{
											inputs_options.string = true;
										}
										else if (boost::iequals(*tokens_input_options_it, std::string("BEGUID")) == 1)
										{
											inputs_options.beguid = true;
										}
										else if (boost::iequals(*tokens_input_options_it, std::string("CHECK")) == 1)
										{
											inputs_options.check = true;
										}
										else if (boost::iequals(*tokens_input_options_it, std::string("NOCHECK")) == 1)
										{
											inputs_options.check = false;
										}
										else
										{
											status = false;
											#ifdef TESTING
												std::cout << "extDB: DB_CUSTOM_V5: Bad Output Option: " << call_name << ":" << *tokens_input_options_it << std::endl;
											#endif
											BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V5: Bad Output Option " << call_name << ":" << *tokens_input_options_it;
										}
									}
								}
								custom_protocol[call_name].sql_inputs_options[sql_line_num].push_back(inputs_options);
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
					std::cout << "extDB: DB_CUSTOM_V5: Template File Missing Incompatible Version" << db_template_file << std::endl;
				#endif
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V5: Template File Missing Incompatible Version: " << db_template_file;
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
// Modified to use lib poco
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


void DB_CUSTOM_V5::getResult(std::unordered_map<std::string, Template_Call>::const_iterator itr, Poco::Data::Statement &sql_statement, std::string &result)
{
	bool sanitize_value_check = true;
	Poco::Data::RecordSet rs(sql_statement);

	result = "[1,[";
	std::size_t cols = rs.columnCount();
	if (cols >= 1)
	{
		bool more = rs.moveFirst();
		while (more)
		{
			result += "[";
			std::size_t sql_output_options_size = itr->second.sql_outputs_options.size();

			for (std::size_t col = 0; col < cols; ++col)
			{
				std::string temp_str = rs[col].convert<std::string>();
				
				// NO OUTPUT OPTIONS 
				if (col >= sql_output_options_size)
				{
					// DEFAULT BEHAVIOUR
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
				// OUTPUT OPTIONS

					// BEGUID
					if (itr-> second.sql_outputs_options[col].beguid)
					{
						getBEGUID(temp_str, temp_str);
						result += temp_str;
					}

					// STRING
					if (itr-> second.sql_outputs_options[col].string)
					{
						if (temp_str.empty())
						{
							result += ("\"\"");
						}
						else
						{
							result += "\"" + temp_str + "\"";
						}
						break;												
					}

					// STRING DATATYPE CHECK
					else if (itr-> second.sql_outputs_options[col].string_datatype_check)
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

					// SANITIZE CHECK
					if (itr-> second.sql_outputs_options[col].check)
					{
						if (!Sqf::check(temp_str))
						{
							sanitize_value_check = false;
						}
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

	if (!(sanitize_value_check))
	{
		result = "[0,\"Error Values Input is not sanitized\"]";
	}
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
			std::cout << "extDB: DB_CUSTOM_V5: Error DBLockedException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error DBLockedException: " + e.displayText();
		result = "[0,\"Error DBLocked Exception\"]";
	}
	catch (Poco::Data::MySQL::ConnectionException& e)
	{
		status = false;
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V5: Error ConnectionException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error ConnectionException: " + e.displayText();
	}
	catch(Poco::Data::MySQL::StatementException& e)
	{
		status = false;
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V5: Error StatementException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error StatementException: " + e.displayText();
		result = "[0,\"Error Statement Exception\"]";
	}
	catch (Poco::Data::DataException& e)
	{
		status = false;
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V5: Error DataException: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error DataException: " + e.displayText();
		result = "[0,\"Error Data Exception\"]";
	}
	catch (Poco::Exception& e)
	{
		status = false;
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V5: Error Exception: " + e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error Exception: " + e.displayText();
		result = "[0,\"Error Exception\"]";
	}
}


void DB_CUSTOM_V5::callCustomProtocol(AbstractExt *extension, std::string call_name, std::unordered_map<std::string, Template_Call>::const_iterator itr, std::vector< std::vector< std::string > > &all_processed_inputs, std::string &input_str, std::string &result)
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

			std::get<1>(db_customSession)[call_name].inputs[i].insert(std::get<1>(db_customSession)[call_name].inputs[i].begin(), all_processed_inputs[i].begin(), all_processed_inputs[i].end());

			Poco::Data::Statement sql_statement(std::get<0>(db_customSession));
			std::get<1>(db_customSession)[call_name].statements.push_back(sql_statement);
			sql_statement << *it_sql_prepared_statements_vector, Poco::Data::use(std::get<1>(db_customSession)[call_name].inputs[i]);
			executeSQL(extension, sql_statement, result, status);

			if (status)
			{
				std::get<1>(db_customSession)[call_name].inputs[i].clear();

				if ( it_sql_prepared_statements_vector+1 == itr->second.sql_prepared_statements.end() )
				{
					getResult(itr, sql_statement, result);
				}
			}
			else
			{
				// Exception Encountered, BREAK + Remove Cache
				std::get<1>(db_customSession).erase(call_name);
				break;
			}
		}
	}
	else
	{
		// CACHE
		for (std::vector<int>::size_type i = 0; i != std::get<1>(db_customSession)[call_name].statements.size(); i++)
		{
			statement_cache_itr->second.inputs[i].insert(statement_cache_itr->second.inputs[i].begin(), all_processed_inputs[i].begin(), all_processed_inputs[i].end());

			executeSQL(extension, statement_cache_itr->second.statements[i], result, status);

			if (status)
			{
				statement_cache_itr->second.inputs[i].clear();
				if (i == (std::get<1>(db_customSession)[call_name].statements.size() - 1))
				{
					getResult(itr, statement_cache_itr->second.statements[i], result);
				}
			}
			else
			{
				// Exception Encountered, BREAK + Remove Cache
				std::get<1>(db_customSession).erase(call_name);
				break;
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


void DB_CUSTOM_V5::callProtocol(AbstractExt *extension, std::string input_str, std::string &result)
{
	#ifdef TESTING
		std::cout << "extDB: DB_CUSTOM_V5: Trace: " + input_str << std::endl;
	#endif
	#ifdef DEBUG_LOGGING
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::trace) << "extDB: DB_CUSTOM_V5: Trace: Input:" + input_str;
	#endif

	Poco::StringTokenizer tokens(input_str, ":");
	std::unordered_map<std::string, Template_Call>::const_iterator itr = custom_protocol.find(tokens[0]);

	if (itr == custom_protocol.end())
	{
		// NO CALLNAME FOUND IN PROTOCOL
		result = "[0,\"Error No Custom Call Not Found\"]";
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: Error No Custom Call Not Found: " + input_str;
	}
	else
	{
		// CALLNAME FOUND IN PROTOCOL
		if (itr->second.number_of_inputs != (tokens.count() - 1))
		{
			// BAD Number of Inputs
			result = "[0,\"Error Incorrect Number of Inputs\"]";
			BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: Incorrect Number of Inputs: " + input_str;
		}
		else
		{
			// GOOD Number of Inputs
			bool bad_chars_detected = false;
			bool sanitize_value_check_ok = true;

			std::vector< std::string > inputs;

			if (itr->second.bad_chars_action > 0)
			{
				// Strip Chars
				std::string temp_str;
				for (std::vector<std::string>::const_iterator token_itr = tokens.begin(); token_itr != tokens.end(); ++token_itr)
				{
					temp_str = *token_itr;
					for (int i = 0; (i < (itr->second.bad_chars.size() - 1)); ++i)
					{
						boost::erase_all(temp_str, std::string(1, itr->second.bad_chars[i]));
					}
					inputs.push_back(temp_str);
					if (temp_str != *token_itr)
					{
						bad_chars_detected = true;
					}
				}
			}
			else
			{
				// DONT Strip Chars
				inputs.insert(inputs.end(), tokens.begin(), tokens.end());
			}

			// Multiple INPUT Lines
			std::vector<std::string>::size_type num_inputs = inputs.size();	
			std::vector<std::string>::size_type num_sql_inputs_options = itr->second.sql_inputs_options.size();

			std::vector< std::vector< std::string > > all_processed_inputs;

			for(int i = 0; i < num_sql_inputs_options; ++i)
			{
				std::vector< std::string > processed_inputs;

				for(int x = 0; x < itr->second.sql_inputs_options[i].size(); ++x)
				{
					std::string temp_str = inputs[itr->second.sql_inputs_options[i][x].number];
					// INPUT Options
						// BEGUID					
					if (itr->second.sql_inputs_options[i][x].beguid)
					{
						getBEGUID(temp_str, temp_str);
					}
						// STRING
					if (itr->second.sql_inputs_options[i][x].string)
					{
						if (temp_str.empty())
						{
							temp_str = ("\"\"");
						}
						else
						{
							temp_str = "\"" + temp_str + "\"";
						}
						break;												
					}

						// SANITIZE CHECK
					if (itr->second.sql_inputs_options[i][x].check)
					{
						if (!Sqf::check(temp_str))
						{
							sanitize_value_check_ok = false;
						}
					}
					processed_inputs.push_back(temp_str);
				}
				all_processed_inputs.push_back(processed_inputs);
			}

			if (!(bad_chars_detected))
			{
				if (sanitize_value_check_ok)
				{
					callCustomProtocol(extension, tokens[0], itr, all_processed_inputs, input_str, result);
				}
				else
				{
					result = "[0,\"Error Values Input is not sanitized\"]";
					BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V5: Sanitize Check error: Input:" + input_str;
				}
			}
			else
			{
				result = "[0,\"Error Bad Char Found\"]";
			}
		}
	}
}