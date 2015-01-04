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


#include "db_custom_v3.h"

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

#include "../sanitize.h"


bool DB_CUSTOM_V3::init(AbstractExt *extension, const std::string init_str)
{
	extension_ptr = extension;

	db_custom_name = init_str;
	
	bool status = false;
	
	if (extension_ptr->getDBType() == std::string("MySQL"))
	{
		status = true;
	}
	else if (extension_ptr->getDBType() == std::string("SQLite"))
	{
		status =  true;
	}
	else
	{
		// DATABASE NOT SETUP YET
		#ifdef TESTING
			extension_ptr->console->warn("extDB: DB_CUSTOM_V3: No Database Connection");
		#endif
		extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: No Database Connection");
		return false;
	}

	// Check if DB_CUSTOM_V3 Template Filename Given
	if (init_str.empty()) 
	{
		#ifdef TESTING
			extension_ptr->console->warn("extDB: DB_CUSTOM_V3: Missing Init Parameter");
		#endif
		extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: Missing Init Parameter");
		return false;
	}
	
	boost::filesystem::path extension_path(extension_ptr->getExtensionPath());
	extension_path /= "extDB";
	extension_path /= "db_custom";

	boost::filesystem::create_directories(extension_path); // Creating Directory if missing
	extension_path /= (init_str + ".ini");
	std::string db_template_file = extension_path.make_preferred().string();

	
	if (boost::filesystem::exists(db_template_file))
	{
		template_ini = (new Poco::Util::IniFileConfiguration(db_template_file));
		
		std::vector < std::string > custom_calls;
		template_ini->keys(custom_calls);

		if (template_ini->hasOption("Default.Version"))
		{
			int default_number_of_inputs = template_ini->getInt("Default.Number of Inputs", 0);

			bool default_string_datatype_check = template_ini->getBool("Default.String Datatype Check", true);
			bool default_sanitize_value_check = template_ini->getBool("Default.Sanitize Value Check", true);

			std::string default_bad_chars_action = template_ini->getString("Default.Bad Chars Action", "STRIP");
			std::string default_bad_chars = template_ini->getString("Default.Bad Chars");

			if ((template_ini->getInt("Default.Version", 1)) == 4)
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
					custom_protocol[call_name].sanitize_value_check = template_ini->getBool(call_name + ".Sanitize Value Check", default_sanitize_value_check);
					custom_protocol[call_name].bad_chars_action = template_ini->getString(call_name + ".Bad Chars Action", default_bad_chars_action);
					custom_protocol[call_name].bad_chars = template_ini->getString(call_name + ".Bad Chars", default_bad_chars);

					while (true)
					{
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

						if ((boost::iequals(custom_protocol[call_name].bad_chars_action, std::string("NONE")) == 1) || 
							(boost::iequals(custom_protocol[call_name].bad_chars_action, std::string("STRIP")) == 1)  ||
							(boost::iequals(custom_protocol[call_name].bad_chars_action, std::string("STRIP+LOG")) == 1)  ||
							(boost::iequals(custom_protocol[call_name].bad_chars_action, std::string("STRIP+ERROR")) == 1)) 
						{					
							std::list<Poco::DynamicAny> sql_list;
							sql_list.push_back(Poco::DynamicAny(sql_str));

							for (int x = (custom_protocol[call_name].number_of_inputs); x > 0; --x)
							{
								std::string input_val_str = "$INPUT_" + Poco::NumberFormatter::format(x);
								size_t input_val_len = input_val_str.length();

								std::string input_stringval_str = "$INPUT_STRING_" + Poco::NumberFormatter::format(x);
								size_t input_stringval_len = input_stringval_str.length();

								std::string input_beguidval_str = "$INPUT_BEGUID_" + Poco::NumberFormatter::format(x);
								size_t input_beguidval_len = input_beguidval_str.length();

								for(std::list<Poco::DynamicAny>::iterator it_sql_list = sql_list.begin(); it_sql_list != sql_list.end(); ++it_sql_list) 
								{
									if (it_sql_list->isString())
									{
										std::string work_str = *it_sql_list;
										std::string tmp_str;
										while (true)
										{
											size_t pos;
											size_t pos2;
											size_t pos3;
											pos = work_str.find(input_val_str);
											pos2 = work_str.find(input_stringval_str);
											pos3 = work_str.find(input_beguidval_str);

											if ((pos < pos2) && (pos < pos3))
											{
												tmp_str = work_str.substr(0, pos);
												work_str = work_str.substr((pos + input_val_len), std::string::npos);
												
												std::list<Poco::DynamicAny>::iterator new_it_sql_vector = sql_list.insert(it_sql_list, x);
												new_it_sql_vector = sql_list.insert(new_it_sql_vector, tmp_str);
											}
											else if ((pos2 < pos) && (pos2 < pos3))
											{
												tmp_str = work_str.substr(0, pos2);
												work_str = work_str.substr((pos2 + input_stringval_len), std::string::npos);
												
												std::list<Poco::DynamicAny>::iterator new_it_sql_vector = sql_list.insert(it_sql_list, -x);
												new_it_sql_vector = sql_list.insert(new_it_sql_vector, tmp_str);
											}
											else if ((pos3 < pos) && (pos3 < pos2))
											{
												tmp_str = work_str.substr(0, pos3);
												work_str = work_str.substr((pos3 + input_beguidval_len), std::string::npos);
												
												std::list<Poco::DynamicAny>::iterator new_it_sql_vector = sql_list.insert(it_sql_list, (-x -1000));
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
							custom_protocol[call_name].sql_statements.push_back(sql_list);
						}
						else
						{
							status = false;
							#ifdef TESTING
								extension_ptr->console->warn("extDB: DB_CUSTOM_V3: Invalid Bad Strings Action for {0} : {1}", call_name, custom_protocol[call_name].bad_chars_action);
							#endif
							extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: Invalid Bad Strings Action for {0} : {1}", call_name, custom_protocol[call_name].bad_chars_action);
						}
					}
				}
			}
			else
			{
				status = false;
				#ifdef TESTING
					extension_ptr->console->warn("extDB: DB_CUSTOM_V3: Template File Missing Incompatiable Version: {0}", db_template_file);
				#endif
				extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: Template File Missing Incompatiable Version: {0}", db_template_file);
			}
		}
		else
		{
			status = false;
			#ifdef TESTING
				extension_ptr->console->warn("extDB: DB_CUSTOM_V3: Template File Missing Default Options: {0}", db_template_file);
			#endif
			extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: Template File Missing Default Options: {0}", db_template_file);
		}
	}
	else 
	{
		status = false;
		#ifdef TESTING
			extension_ptr->console->warn("extDB: DB_CUSTOM_V3: No Template File Found: {0}", db_template_file);
		#endif
		extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: No Template File Found: {0}", db_template_file);
	}
	return status;
}


void DB_CUSTOM_V3::getBEGUID(std::string &input_str, std::string &result)
// From Frank https://gist.github.com/Fank/11127158
// Modified to use libpoco
{
	boost::lock_guard<boost::mutex> lock(mutex_md5);
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
		Poco::Int8 i = 0, parts[8] = { 0 };

		do
		{
			parts[i++] = steamID & 0xFFu;
		} while (steamID >>= 8);

		std::stringstream bestring;
		bestring << "BE";
		for (int i = 0; i < sizeof(parts); i++) {
			bestring << char(parts[i]);
		}

		md5.update(bestring.str());
		result = ("\"" + Poco::DigestEngine::digestToHex(md5.digest()) + "\"");
	}
}


void DB_CUSTOM_V3::callCustomProtocol(boost::unordered_map<std::string, Template_Calls>::const_iterator itr, std::vector< std::string > &tokens, bool &sanitize_value_check_ok, std::string &result)
{
	result.clear();
	
	Poco::Data::Session session = extension_ptr->getDBSession_mutexlock();
	Poco::Data::Statement sql_current(session);

	for(std::vector< std::list<Poco::DynamicAny> >::const_iterator it_sql_statements_vector = itr->second.sql_statements.begin(); it_sql_statements_vector != itr->second.sql_statements.end(); ++it_sql_statements_vector)
	{
		std::string sql_str;
		std::string tmp_str;

		for(std::list<Poco::DynamicAny>::const_iterator it_sql_list = it_sql_statements_vector->begin(); it_sql_list != it_sql_statements_vector->end(); ++it_sql_list) 
		{
			if (it_sql_list->isString())  // Check for $Input Variable
			{
				sql_str += it_sql_list->convert<std::string>();
			}
			else
			{
				if (*it_sql_list > 0) // Convert $Input to String
				{
					if (itr->second.sanitize_value_check)
					{
						// Sanitize Check
						if (!Sqf::check(tokens[*it_sql_list]))
						{
							sanitize_value_check_ok = false;
							break;
						}
					}
					sql_str += tokens[*it_sql_list]; 
				}
				else if (*it_sql_list > -1000) // Convert $Input to String ""
				{
					tmp_str = tokens[(-1 * *it_sql_list)];
					boost::erase_all(tmp_str, "\"");
					tmp_str = "\"" + tmp_str + "\"";

					if (itr->second.sanitize_value_check)
					{
						// Sanitize Check
						if (!Sqf::check(tmp_str))
						{
							sanitize_value_check_ok = false;
							break;
						}
					}

					sql_str += tmp_str;
				}
				else // Convert $Input to "BEGUID"
				{
					std::string input_str_test = tokens[((-1 * *it_sql_list) - 1000)];
					getBEGUID(input_str_test, tmp_str);
					sql_str += tmp_str;
				}
			}
		}

		if (sanitize_value_check_ok)
		{
			try 
			{
				Poco::Data::Statement sql(session);
				sql << sql_str;
				sql.execute();
				sql_current.swap(sql);
			}
			catch (Poco::InvalidAccessException& e)
			{
				#ifdef TESTING
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error InvalidAccessException: {0}", e.displayText());
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error InvalidAccessException: SQL: {0}", sql_str);
				#endif
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error InvalidAccessException: {0}", e.displayText());
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error InvalidAccessException: SQL: {0}", sql_str);
				result = "[0,\"Error DBLocked Exception\"]";
				break;
			}
			catch (Poco::Data::NotConnectedException& e)
			{
				#ifdef TESTING
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error NotConnectedException: {0}", e.displayText());
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error NotConnectedException: SQL: {0}", sql_str);
				#endif
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error NotConnectedException: {0}", e.displayText());
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error NotConnectedException: SQL: {0}", sql_str);
				result = "[0,\"Error DBLocked Exception\"]";
				break;
			}
			catch (Poco::NotImplementedException& e)
			{
				#ifdef TESTING
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error NotImplementedException: {0}", e.displayText());
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error NotImplementedException: SQL: {0}", sql_str);

				#endif
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error NotImplementedException: {0}", e.displayText());
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error NotImplementedException: SQL: {0}", sql_str);
				result = "[0,\"Error DBLocked Exception\"]";
				break;
			}
			catch (Poco::Data::SQLite::DBLockedException& e)
			{
				#ifdef TESTING
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error DBLockedException: {0}", e.displayText());
					extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error DBLockedException: SQL: {0}", sql_str);
				#endif
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error DBLockedException: {0}", e.displayText());
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error DBLockedException: SQL: {0}", sql_str);
				result = "[0,\"Error DBLocked Exception\"]";
				break;
			}
			catch (Poco::Data::MySQL::ConnectionException& e)
			{
				#ifdef TESTING
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error ConnectionException: {0}", e.displayText());
					extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error ConnectionException: SQL: {0}", sql_str);
				#endif
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error ConnectionException: {0}", e.displayText());
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error ConnectionException: SQL: {0}", sql_str);
				result = "[0,\"Error Connection Exception\"]";
				break;
			}
			catch(Poco::Data::MySQL::StatementException& e)
			{
				#ifdef TESTING
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error StatementException: {0}", e.displayText());
					extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error StatementException: SQL: {0}", sql_str);
				#endif
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error StatementException: {0}", e.displayText());
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error StatementException: SQL: {0}", sql_str);
				result = "[0,\"Error Statement Exception\"]";
				break;
			}
			catch (Poco::Data::DataException& e)
			{
				#ifdef TESTING
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error DataException: {0}", e.displayText());
					extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error DataException: SQL: {0}", sql_str);
				#endif
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error DataException: {0}", e.displayText());
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error DataException: SQL: {0}", sql_str);
				result = "[0,\"Error Data Exception\"]";
				break;
			}
			catch (Poco::Exception& e)
			{
				#ifdef TESTING
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error Exception: {0}", e.displayText());
					extension_ptr->console->error("extDB: DB_CUSTOM_V3: Error Exception: SQL: {0}", sql_str);
				#endif
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error Exception: {0}", e.displayText());
				extension_ptr->logger->error("extDB: DB_CUSTOM_V3: Error Exception: SQL: {0}", sql_str);
				result = "[0,\"Error Exception\"]";
				break;
			}
		}
		else
		{
			break;
		}
	}

	if (result.empty())
	{
		Poco::Data::RecordSet rs(sql_current);

		result = "[1,[";
		std::size_t cols = rs.columnCount();
		if (cols >= 1)
		{
			bool more = rs.moveFirst();
			while (more)
			{
				result += "[";
				for (std::size_t col = 0; col < cols; ++col)
				{
					std::string temp_str = rs[col].convert<std::string>();

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
	
	#ifdef TESTING
		extension_ptr->console->info("extDB: DB_CUSTOM_V3: Trace: Result: {0}", result);
	#endif
	#ifdef DEBUG_LOGGING
		extension_ptr->logger->info("extDB: DB_CUSTOM_V3: Trace: Result: {0}", result);
	#endif
}


void DB_CUSTOM_V3::callProtocol(std::string input_str, std::string &result)
{
	#ifdef TESTING
		extension_ptr->console->info("extDB: DB_CUSTOM_V3: Trace: Input: {0}", input_str);
	#endif
	#ifdef DEBUG_LOGGING
		extension_ptr->logger->info("extDB: DB_CUSTOM_V3: Trace: Input: {0}", input_str);
	#endif

	Poco::StringTokenizer tokens(input_str, ":");
	
	int token_count = tokens.count();
	boost::unordered_map<std::string, Template_Calls>::const_iterator itr = custom_protocol.find(tokens[0]);
	if (itr == custom_protocol.end())
	{
		result = "[0,\"Error No Custom Call Not Found\"]";
		extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: Error No Custom Call Not Found: {0}", input_str);
	}
	else
	{
		if (itr->second.number_of_inputs != (token_count - 1))
		{
			result = "[0,\"Error Incorrect Number of Inputs\"]";
			extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: Incorrect Number of Inputs: {0}", input_str);
		}
		else
		{
			std::vector< std::string > inputs;
			inputs.push_back(""); // We ignore [0] Entry, makes logic simplier when using -x value to indicate $INPUT_STRING_X
			std::string input_value_str;

			bool bad_chars_error = false;
			if (boost::iequals(itr->second.bad_chars_action, std::string("STRIP")) == 1)
			{
				for(int i = 1; i < token_count; ++i) 
				{
					input_value_str = tokens[i];

					// Strip Chars					
					for (int i2 = 0; (i2 < (itr->second.bad_chars.size() - 1)); ++i2)
					{
						boost::erase_all(input_value_str, std::string(1,itr->second.bad_chars[i2]));
					}

					// Add String to List
					inputs.push_back(input_value_str);	
				}
			}
			else if (boost::iequals(itr->second.bad_chars_action, std::string("STRIP+LOG")) == 1)
			{
				for(int i = 1; i < token_count; ++i) 
				{
					input_value_str = tokens[i];

					// Strip Chars					
					for (int i2 = 0; (i2 < (itr->second.bad_chars.size() - 1)); ++i2)
					{
						boost::erase_all(input_value_str, std::string(1,itr->second.bad_chars[i2]));
					}

					if (input_value_str != tokens[i])
					{
						extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: Error Bad Char Detected: Input: {0}", input_str);
						extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: Error Bad Char Detected: Token: {0}", tokens[i]);
					}

					// Add String to List
					inputs.push_back(input_value_str);	
				}
			}
			else if (boost::iequals(itr->second.bad_chars_action, std::string("STRIP+ERROR")) == 1)
			{
				for(int i = 1; i < token_count; ++i) 
				{
					input_value_str = tokens[i];

					// Strip Chars					
					for (int i2 = 0; (i2 < (itr->second.bad_chars.size() - 1)); ++i2)
					{
						boost::erase_all(input_value_str, std::string(1,itr->second.bad_chars[i2]));
					}

					if (input_value_str != tokens[i])
					{
						extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: Error Bad Char Detected: Input: {0}", input_str);
						extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: Error Bad Char Detected: Token: {0}", tokens[i]);
						bad_chars_error = true;
					}

					// Add String to List
					inputs.push_back(input_value_str);	
				}
			}
			else
			{
				for(int i = 1; i < token_count; ++i) 
				{
					inputs.push_back(tokens[i]);	
				}
			}

			if (!(bad_chars_error))
			{
				bool sanitize_value_check_ok = true;
				callCustomProtocol(extension, itr, inputs, sanitize_value_check_ok, result);
				if (!sanitize_value_check_ok)
				{
					result = "[0,\"Error Values Input is not sanitized\"]";
					extension_ptr->logger->warn("extDB: DB_CUSTOM_V3: Sanitize Check error: Input: {0}", input_str);
				}
			}
			else
			{
				result = "[0,\"Error Bad Char Found\"]";
			}
		}
	}
}