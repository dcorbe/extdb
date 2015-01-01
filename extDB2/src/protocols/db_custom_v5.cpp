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
			extension->console->warn("extDB: DB_CUSTOM_V5: No Database Connection");
		#endif
		extension->logger->warn("extDB: DB_CUSTOM_V5: No Database Connection");
		return false;
	}

	// Check if DB_CUSTOM_V5 Template Filename Given
	if (init_str.empty()) 
	{
		#ifdef TESTING
			extension->console->warn("extDB: DB_CUSTOM_V5: Missing Init Parameter");
		#endif
		extension->logger->warn("extDB: DB_CUSTOM_V5: Missing Init Parameter");
		return false;
	}

	boost::filesystem::path extension_path(extension->getExtensionPath());
	extension_path /= "extDB";
	extension_path /= "db_custom";

	boost::filesystem::create_directories(extension_path); // Creating Directory if missing
	extension_path /= (init_str + ".ini");
	std::string db_template_file = extension_path.make_preferred().string();

	#ifdef TESTING
		extension->console->info("extDB: DB_CUSTOM_V5: Loading Template Filename: {0}", db_template_file);
	#endif
	extension->logger->info("extDB: DB_CUSTOM_V5: Loading Template Filename: {0}", db_template_file);
	
	// Read Template File
	if (boost::filesystem::exists(db_template_file))
	{
		template_ini = (new Poco::Util::IniFileConfiguration(db_template_file));
		
		std::vector < std::string > custom_calls;
		template_ini->keys(custom_calls);

		if (template_ini->hasOption("Default.Version"))
		{
			int default_number_of_inputs = template_ini->getInt("Default.Number of Inputs", 0);

			bool default_input_sanitize_value_check = template_ini->getBool("Default.Sanitize Input Value Check", true);
			bool default_output_sanitize_value_check = template_ini->getBool("Default.Sanitize Output Value Check", true);
			bool default_string_datatype_check = template_ini->getBool("Default.String Datatype Check", true);
			bool default_preparedStatement_cache = template_ini->getBool("Default.Prepared Statement Cache", true);

			bool default_strip = template_ini->getBool("Default.Strip", true);
			std::string default_strip_chars = template_ini->getString("Default.Strip Chars", "");
			int default_strip_chars_action = 0;

			std::string strip_chars_action_str = template_ini->getString("Default.Strip Chars Action", "Strip");
			if	(boost::iequals(strip_chars_action_str, std::string("Strip")) == 1)
			{
				default_strip_chars_action = 1;
			}
			else if	(boost::iequals(strip_chars_action_str, std::string("Strip+Log")) == 1)
			{
				default_strip_chars_action = 2;
			}
			else if	(boost::iequals(strip_chars_action_str, std::string("Strip+Error")) == 1)
			{
				default_strip_chars_action = 3;
			}
			else if (boost::iequals(strip_chars_action_str, std::string("None")) == 1)
			{
				default_strip_chars_action = 0;
			}
			else
			{
				#ifdef TESTING
					extension->console->warn("extDB: DB_CUSTOM_V5: Invalid Default Strip Chars Action: {0}", strip_chars_action_str);
				#endif
				extension->logger->warn("extDB: DB_CUSTOM_V5: Invalid Default Strip Chars Action: {0}", strip_chars_action_str);
			}

			if ((template_ini->getInt("Default.Version", 1)) == 7)
			{
				for(std::vector<std::string>::iterator it = custom_calls.begin(); it != custom_calls.end(); ++it) 
				{
					int sql_line_num = 0;
					int sql_part_num = 0;
					std::string call_name = *it;

					std::string sql_line_num_str;
					std::string sql_part_num_str;

					custom_protocol[call_name].number_of_inputs = template_ini->getInt(call_name + ".Number of Inputs", default_number_of_inputs);
					custom_protocol[call_name].string_datatype_check = template_ini->getBool(call_name + ".String Datatype Check", default_string_datatype_check);
					custom_protocol[call_name].preparedStatement_cache = template_ini->getBool("Default.Prepared Statement Cache", default_preparedStatement_cache);

					if (template_ini->has(call_name + ".Strip Chars Action"))
					{
						strip_chars_action_str = template_ini->getString(call_name + ".Strip Chars Action", "");
						if	(boost::iequals(strip_chars_action_str, std::string("Strip")) == 1)
						{
							custom_protocol[call_name].strip_chars_action = 1;
						}
						else if	(boost::iequals(strip_chars_action_str, std::string("Strip+Log")) == 1)
						{
							custom_protocol[call_name].strip_chars_action = 2;
						}
						else if	(boost::iequals(strip_chars_action_str, std::string("Strip+Error")) == 1)
						{
							custom_protocol[call_name].strip_chars_action = 3;
						}
						else if (boost::iequals(strip_chars_action_str, std::string("None")) == 1)
						{
							custom_protocol[call_name].strip_chars_action = 0;
						}
						else
						{
							#ifdef TESTING
								extension->console->warn("extDB: DB_CUSTOM_V5: Invalid Strip Chars Action: {0}", strip_chars_action_str);
							#endif
							extension->logger->warn("extDB: DB_CUSTOM_V5: Invalid Strip Chars Action: {0}", strip_chars_action_str);
						}
					}
					else
					{
						custom_protocol[call_name].strip_chars_action = default_strip_chars_action;
					}
					
					custom_protocol[call_name].strip = template_ini->getBool(call_name + ".Strip", default_strip);
					custom_protocol[call_name].strip_chars = template_ini->getString(call_name + ".Strip Chars", default_strip_chars);

					custom_protocol[call_name].input_sanitize_value_check = template_ini->getBool(call_name + ".Sanitize Value Check", default_input_sanitize_value_check);
					custom_protocol[call_name].output_sanitize_value_check = template_ini->getBool(call_name + ".Sanitize Value Check", default_output_sanitize_value_check);

					while (true)
					{
						if (custom_protocol[call_name].strip_chars_action > 0)
						{
							// Prepared Statement
							++sql_line_num;
							sql_line_num_str = Poco::NumberFormatter::format(sql_line_num);

							if (!(template_ini->has(call_name + ".SQL" + sql_line_num_str + "_1")))  // No More SQL Statements
							{
								// Initialize Default Output Options

								// Get Output Options
								Poco::StringTokenizer tokens_output_options((template_ini->getString(call_name + ".OUTPUT", "")), ",", Poco::StringTokenizer::TOK_TRIM);

								for (int i = 0; i < (tokens_output_options.count()); ++i)
								{
									Value_Options outputs_options;
									outputs_options.check = default_output_sanitize_value_check;

									Poco::StringTokenizer options_tokens(tokens_output_options[i], "-", Poco::StringTokenizer::TOK_TRIM);
									for (int x = 0; x < (options_tokens.count()); ++x)
									{
										if (!(Poco::NumberParser::tryParse(options_tokens[x], outputs_options.number)))
										{
											if (boost::iequals(options_tokens[x], std::string("String")) == 1)
											{
												outputs_options.string = true;
											}
											else if (boost::iequals(options_tokens[x], std::string("BeGUID")) == 1)
											{
												outputs_options.beguid = true;
											}
											else if (boost::iequals(options_tokens[x], std::string("Check")) == 1)
											{
												outputs_options.check = true;
											}
											else if (boost::iequals(options_tokens[x], std::string("NoCheck")) == 1)
											{
												outputs_options.check = false;
											}
											else if (boost::iequals(options_tokens[x], std::string("Strip")) == 1)
											{
												outputs_options.strip = true;
											}
											else if (boost::iequals(options_tokens[x], std::string("NoStrip")) == 1)
											{
												outputs_options.strip = false;
											}
											else if (boost::iequals(options_tokens[x], std::string("AltisLifeRPG_Array")) == 1)
											{
												outputs_options.toArray_AltisLifeRpg = true;
											}
											else
											{
												status = false;
												#ifdef TESTING
													extension->console->warn("extDB: DB_CUSTOM_V5: Invalid Strip Output Option: {0}: {1}", call_name, options_tokens[x]);
												#endif
												extension->logger->warn("extDB: DB_CUSTOM_V5: Invalid Strip Output Option: {0}: {1}", call_name, options_tokens[x]);
											}
										}
									}
									custom_protocol[call_name].sql_outputs_options.push_back(std::move(outputs_options));
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

							custom_protocol[call_name].sql_prepared_statements.push_back(std::move(sql_str));

							// Get Input Options
							Poco::StringTokenizer tokens_input(template_ini->getString((call_name + ".SQL" + sql_line_num_str + "_INPUTS"), ""), ",");

							// Initialize Default Input Options
							Value_Options inputs_options;
							inputs_options.check = default_input_sanitize_value_check;

							custom_protocol[call_name].sql_inputs_options.push_back(std::vector < Value_Options >());

							for (int x = 0; x < (tokens_input.count()); x++)
							{
								Poco::StringTokenizer tokens_input_options(tokens_input[x], "-");
								
								for (Poco::StringTokenizer::Iterator tokens_input_options_it = tokens_input_options.begin(); tokens_input_options_it != tokens_input_options.end(); ++tokens_input_options_it)
								{
									if (!(Poco::NumberParser::tryParse(*tokens_input_options_it, inputs_options.number)))
									{
										if (boost::iequals(*tokens_input_options_it, std::string("String")) == 1)
										{
											inputs_options.string = true;
										}
										else if (boost::iequals(*tokens_input_options_it, std::string("BeGUID")) == 1)
										{
											inputs_options.beguid = true;
										}
										else if (boost::iequals(*tokens_input_options_it, std::string("Check")) == 1)
										{
											inputs_options.check = true;
										}
										else if (boost::iequals(*tokens_input_options_it, std::string("NoCheck")) == 1)
										{
											inputs_options.check = false;
										}
										else if (boost::iequals(*tokens_input_options_it, std::string("Strip")) == 1)
										{
											inputs_options.strip = true;
										}
										else if (boost::iequals(*tokens_input_options_it, std::string("NoStrip")) == 1)
										{
											inputs_options.strip = false;
										}
										else if (boost::iequals(*tokens_input_options_it, std::string("AltisLifeRPG_Array")) == 1)
										{
											inputs_options.toArray_AltisLifeRpg = true;
										}
										else
										{
											status = false;
											#ifdef TESTING
												extension->console->warn("extDB: DB_CUSTOM_V5: Invalid Strip Input Option: {0}: {1}", call_name, *tokens_input_options_it);
											#endif
												extension->logger->warn("extDB: DB_CUSTOM_V5: Invalid Strip Input Option: {0}: {1}", call_name, *tokens_input_options_it);
										}
									}
								}
								custom_protocol[call_name].sql_inputs_options[sql_line_num - 1].push_back(inputs_options);
							}
						}
						else
						{
							status = false;
							#ifdef TESTING
								extension->console->warn("extDB: DB_CUSTOM_V5: Invalid Strip Strings Action for : {0}: {1}", call_name, custom_protocol[call_name].strip_chars_action);
							#endif
							extension->logger->warn("extDB: DB_CUSTOM_V5: Invalid Strip Strings Action for : {0}: {1}", call_name, custom_protocol[call_name].strip_chars_action);
						}
					}
				}
			}
			else
			{
				status = false;
				#ifdef TESTING
					extension->console->warn("extDB: DB_CUSTOM_V5: Template File Incompatible Version: {0}", db_template_file);
				#endif
				extension->logger->warn("extDB: DB_CUSTOM_V5: Template File Incompatible Version: {0}", db_template_file);
			}
		}
		else
		{
			status = false;
			#ifdef TESTING
				extension->console->warn("extDB: DB_CUSTOM_V5: Template File Missing Default Options: {0}", db_template_file);
			#endif
			extension->logger->warn("extDB: DB_CUSTOM_V5: Template File Missing Default Options: {0}", db_template_file);
		}
	}
	else
	{
		status = false;
		#ifdef TESTING
			extension->console->warn("extDB: DB_CUSTOM_V5: Template File Not Found: {0}", db_template_file);
		#endif
		extension->logger->warn("extDB: DB_CUSTOM_V5: Template File Not Found: {0}", db_template_file);
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

void DB_CUSTOM_V5::toArrayAltisLifeRpg(std::string &input_str, std::string &result, bool toArray)
// Function to act as Altis-Life fn_mresArray / fn_mresToArray
{
	if (toArray)
	{
		result = Poco::replace(input_str, "`", "\"");
	}
	else
	{
		result = Poco::replace(input_str, "\"", "`");
	}
}


void DB_CUSTOM_V5::getResult(std::unordered_map<std::string, PS_Template_Call>::const_iterator itr, Poco::Data::Statement &sql_statement, std::string &result)
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
					if ((itr->second.string_datatype_check) && (rs.columnType(col) == Poco::Data::MetaColumn::FDT_STRING))
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
					else if (temp_str.empty())
					{
						result += ("\"\"");
					}
					else
					{
						result += temp_str;
					}
				}
				else
				{
				// OUTPUT OPTIONS

					if (itr->second.sql_outputs_options[col].toArray_AltisLifeRpg)
					{
						toArrayAltisLifeRpg(temp_str, temp_str, true);
					}

					// BEGUID
					if (itr->second.sql_outputs_options[col].beguid)
					{
						getBEGUID(temp_str, temp_str);
					}

					// STRING
					if (itr->second.sql_outputs_options[col].string)
					{
						if (temp_str.empty())
						{
							temp_str = ("\"\"");
						}
						else
						{
							boost::erase_all(temp_str, "\"");
							temp_str = "\"" + temp_str + "\"";
						}
					}

					// STRING DATATYPE CHECK
					else if ((itr->second.sql_outputs_options[col].string_datatype_check) && (rs.columnType(col) == Poco::Data::MetaColumn::FDT_STRING))
					{
						if (temp_str.empty())
						{
							temp_str = ("\"\"");
						}
						else
						{
							boost::erase_all(temp_str, "\"");
							temp_str = "\"" + temp_str + "\"";
						}
					}
					else
					{
						if (temp_str.empty())
						{
							temp_str = ("\"\"");
						}
					}						

					// SANITIZE CHECK
					if (itr->second.sql_outputs_options[col].check)
					{
						if (!Sqf::check(temp_str))
						{
							sanitize_value_check = false;
						}
					}
					result += temp_str;
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
	catch (Poco::InvalidAccessException& e)
	{
		status = false;
		#ifdef TESTING
			extension->console->error("extDB: DB_CUSTOM_V5: Error NotConnectedException: {0}", e.displayText());
		#endif
		extension->logger->error("extDB: DB_CUSTOM_V5: Error NotConnectedException: {0}", e.displayText());
		result = "[0,\"Error DBLocked Exception\"]";
	}
	catch (Poco::Data::NotConnectedException& e)
	{
		status = false;
		#ifdef TESTING
			extension->console->error("extDB: DB_CUSTOM_V5: Error NotConnectedException: {0}", e.displayText());
		#endif
		extension->logger->error("extDB: DB_CUSTOM_V5: Error NotConnectedException: {0}", e.displayText());
		result = "[0,\"Error DBLocked Exception\"]";
	}
	catch (Poco::NotImplementedException& e)
	{
		status = false;
		#ifdef TESTING
			extension->console->error("extDB: DB_CUSTOM_V5: Error NotImplementedException: {0}", e.displayText());
		#endif
		extension->logger->error("extDB: DB_CUSTOM_V5: Error NotImplementedException: {0}", e.displayText());
		result = "[0,\"Error DBLocked Exception\"]";
	}
	catch (Poco::Data::SQLite::DBLockedException& e)
	{
		status = false;
		#ifdef TESTING
			extension->console->error("extDB: DB_CUSTOM_V5: Error DBLockedException: {0}", e.displayText());
		#endif
		extension->logger->error("extDB: DB_CUSTOM_V5: Error DBLockedException: {0}", e.displayText());
		result = "[0,\"Error DBLocked Exception\"]";
	}
	catch (Poco::Data::MySQL::ConnectionException& e)
	{
		status = false;
		#ifdef TESTING
			extension->console->error("extDB: DB_CUSTOM_V5: Error ConnectionException: {0}", e.displayText());
		#endif
		extension->logger->error("extDB: DB_CUSTOM_V5: Error ConnectionException: {0}", e.displayText());
		result = "[0,\"Error Connection Exception\"]";
	}
	catch(Poco::Data::MySQL::StatementException& e)
	{
		status = false;
		#ifdef TESTING
			extension->console->error("extDB: DB_CUSTOM_V5: Error StatementException: {0}", e.displayText());
		#endif
		extension->logger->error("extDB: DB_CUSTOM_V5: Error StatementException: {0}", e.displayText());
		result = "[0,\"Error Statement Exception\"]";
	}
	catch (Poco::Data::DataException& e)
	{
		status = false;
		#ifdef TESTING
			extension->console->error("extDB: DB_CUSTOM_V5: Error DataException: {0}", e.displayText());
		#endif
		extension->logger->error("extDB: DB_CUSTOM_V5: Error DataException: {0}", e.displayText());
		result = "[0,\"Error Data Exception\"]";
	}
	catch (Poco::Exception& e)
	{
		status = false;
		#ifdef TESTING
			extension->console->error("extDB: DB_CUSTOM_V5: Error Exception: {0}", e.displayText());
		#endif
		extension->logger->error("extDB: DB_CUSTOM_V5: Error Exception: {0}", e.displayText());
		result = "[0,\"Error Exception\"]";
	}
}


void DB_CUSTOM_V5::callCustomProtocol(AbstractExt *extension, std::string call_name, std::unordered_map<std::string, PS_Template_Call>::const_iterator itr, std::vector< std::vector< std::string > > &all_processed_inputs, std::string &input_str, std::string &result)
{
	boost::lock_guard<boost::mutex> lock(extension->mutex_poco_cached_preparedStatements);

	bool status = true;

	Poco::Data::SessionPool::SessionDataPtr session_data_ptr;
	Poco::Data::Session session = extension->getDBSession_mutexlock(session_data_ptr);

	std::unordered_map <std::string, Poco::Data::SessionPool::StatementCache>::iterator statement_cache_itr = session_data_ptr->statements_map.find(call_name);
	if (statement_cache_itr == session_data_ptr->statements_map.end())
	{
		extension->console->info("extDB: DB_CUSTOM_V5: NO CACHED");
		// NO CACHE

		int i = -1;
		for (std::vector< std::string >::const_iterator it_sql_prepared_statements_vector = itr->second.sql_prepared_statements.begin(); it_sql_prepared_statements_vector != itr->second.sql_prepared_statements.end(); ++it_sql_prepared_statements_vector)
		{
			i++;

			Poco::Data::Statement sql_statement(session);

			if (itr->second.number_of_inputs == 0)
			{
				sql_statement << *it_sql_prepared_statements_vector;
			}
			else
			{
				sql_statement << *it_sql_prepared_statements_vector;
				for (int x = 0; x < all_processed_inputs[i].size(); x++)
				{
					sql_statement, Poco::Data::Keywords::use(all_processed_inputs[i][x]);
				}
			}

			executeSQL(extension, sql_statement, result, status);

			if (status)
			{
				if ( it_sql_prepared_statements_vector+1 == itr->second.sql_prepared_statements.end() )
				{
					getResult(itr, sql_statement, result);
				}
				if (itr->second.preparedStatement_cache)
				{
					session_data_ptr->statements_map[call_name].push_back(std::move(sql_statement));
				}
			}
			else
			{
				session_data_ptr->statements_map.erase(call_name);
				break;
			}
		}
	}
	else
	{
		extension->console->info("extDB: DB_CUSTOM_V5: CACHED");
		// CACHE
		for (std::vector<int>::size_type i = 0; i != statement_cache_itr->second.size(); i++)
		{
			statement_cache_itr->second[i].bindClear();
			for (int x = 0; x < all_processed_inputs[i].size(); x++)
			{
				statement_cache_itr->second[i], Poco::Data::Keywords::use(all_processed_inputs[i][x]);
			}
			statement_cache_itr->second[i].bindFixup();

			executeSQL(extension, statement_cache_itr->second[i], result, status);

			if (status)
			{
				if (i == (statement_cache_itr->second.size() - 1))
				{
					getResult(itr, statement_cache_itr->second[i], result);
				}
			}
			else
			{
				// Exception Encountered, Break + Remove Cache
				session_data_ptr->statements_map.erase(call_name);
				break;
			}
		}
	}

	if (!status)
	{
		extension->logger->warn("extDB: DB_CUSTOM_V5: Error Exception: SQL: {0}", input_str);
	}
	else
	{
		#ifdef TESTING
			extension->console->info("extDB: DB_CUSTOM_V5: Trace: Result: {0}", result);
		#endif
		#ifdef DEBUG_LOGGING
			extension->logger->info("extDB: DB_CUSTOM_V5: Trace: Result: {0}", result);
		#endif
	}
}


void DB_CUSTOM_V5::callProtocol(AbstractExt *extension, std::string input_str, std::string &result)
{
	#ifdef TESTING
		extension->console->info("extDB: DB_CUSTOM_V5: Trace: Input: {0}", input_str);
	#endif
	#ifdef DEBUG_LOGGING
		extension->logger->info("extDB: DB_CUSTOM_V5: Trace: Input: {0}", input_str);
	#endif

	Poco::StringTokenizer tokens(input_str, ":");
	std::unordered_map<std::string, PS_Template_Call>::const_iterator itr = custom_protocol.find(tokens[0]);

	if (itr == custom_protocol.end())
	{
		// NO CALLNAME FOUND IN PROTOCOL
		result = "[0,\"Error No Custom Call Not Found\"]";
		extension->logger->warn("extDB: DB_CUSTOM_V5: Error No Custom Call Not Found: {0}", input_str);
	}
	else
	{
		// CALLNAME FOUND IN PROTOCOL
		if (itr->second.number_of_inputs != (tokens.count() - 1))
		{
			// BAD Number of Inputs
			result = "[0,\"Error Incorrect Number of Inputs\"]";
			extension->logger->warn("extDB: DB_CUSTOM_V5:Incorrect Number of Inputs: {0}", input_str);
		}
		else
		{
			// GOOD Number of Inputs
			bool abort_status = false;
			bool strip_chars_detected = false;

			std::vector< std::string > inputs (tokens.begin(), tokens.end());

			// Multiple INPUT Lines
			std::vector<std::string>::size_type num_inputs = inputs.size();	
			std::vector<std::string>::size_type num_sql_inputs_options = itr->second.sql_inputs_options.size();

			std::vector< std::vector<std::string> > all_processed_inputs;

			for(int i = 0; i < num_sql_inputs_options; ++i)
			{
				std::vector< std::string > processed_inputs;
				processed_inputs.clear();

				if (itr->second.number_of_inputs > 0)
				{
					for(int x = 0; x < itr->second.sql_inputs_options[i].size(); ++x)
					{
						std::string temp_str = inputs[itr->second.sql_inputs_options[i][x].number];
						// INPUT Options
							// Strip
						if (itr->second.sql_inputs_options[i][x].strip)
						{
							for (int y = 0; (y < (itr->second.strip_chars.size() - 1)); ++y)
							{
								boost::erase_all(temp_str, std::string(1,itr->second.strip_chars[y]));
							}
							if (temp_str != inputs[itr->second.sql_inputs_options[i][x].number])
							{
								strip_chars_detected = true;
								switch (itr->second.strip_chars_action)
								{
									case 3: // Strip + Log + Error
										abort_status = true;	
									case 2: // Strip + Log
										extension->logger->warn("extDB: DB_CUSTOM_V5: Error Bad Char Detected: Input: {0}", input_str);
										extension->logger->warn("extDB: DB_CUSTOM_V5: Error Bad Char Detected: Token: {0}", inputs[itr->second.sql_inputs_options[i][x].number]);
									case 1: // Strip
										result = "[0,\"Error Strip Char Found\"]";
										break;
								}
							}
						}
							// ToArray AltisLifeRPG
						if (itr->second.sql_inputs_options[i][x].beguid)
						{
							toArrayAltisLifeRpg(temp_str, temp_str, false);	
						}
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
						}

							// SANITIZE CHECK
						if (itr->second.sql_inputs_options[i][x].check)
						{
							abort_status = Sqf::check(temp_str);
							extension->logger->warn("extDB: DB_CUSTOM_V5: Sanitize Check error: Input: {0}", input_str);
							result = "[0,\"Error Values Input is not sanitized\"]";
						}
						processed_inputs.push_back(std::move(temp_str));
					}
				}
				if (abort_status)
				{
					break;
				}
				all_processed_inputs.push_back(std::move(processed_inputs));
			}

			if (!abort_status)
			{
				callCustomProtocol(extension, tokens[0], itr, all_processed_inputs, input_str, result);
			}
		}
	}
}
