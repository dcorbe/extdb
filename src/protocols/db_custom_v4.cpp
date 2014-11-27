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

#include "db_custom_v4.h"

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


bool DB_CUSTOM_V4::init(AbstractExt *extension, const std::string init_str)
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
			std::cout << "extDB: DB_CUSTOM_V4: No Database Connection" << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: No Database Connection";
		return false;
	}

	// Check if DB_CUSTOM_V4 Template Filename Given
	if (init_str.empty()) 
	{
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V4: Missing Parameter or No Template Filename given" << std::endl;
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
			int default_number_of_inputs = template_ini->getInt("Default.Number of Inputs", 0);

			bool default_string_datatype_check = template_ini->getBool("Default.String Datatype Check", true);
			bool default_sanitize_value_check = template_ini->getBool("Default.Sanitize Value Check", true);

			std::string default_bad_chars_action = template_ini->getString("Default.Bad Chars Action", "STRIP");
			std::string default_bad_chars = template_ini->getString("Default.Bad Chars");

			if ((template_ini->getInt("Default.Version", 1)) == 5)
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

						std::vector<int> sql_output;
						Poco::StringTokenizer tokens((template_ini->getString(call_name + ".OUTPUT", "")), ",");


						for (int x = 0; x < (tokens.count()); x++)
						{
							// 0 Nothing
							// 1 Add Quotes
							// 2 Auto String Check

							if ((boost::iequals(tokens[x], std::string("N")) == 1) || (boost::iequals(tokens[x], std::string("Nothing")) == 1))
							{
								sql_output.push_back(0);
							}
							else if ((boost::iequals(tokens[x], std::string("S")) == 1) || (boost::iequals(tokens[x], std::string("String")) == 1))
							{
								sql_output.push_back(1);
							}
							else if ((boost::iequals(tokens[x], std::string("A")) == 1) || (boost::iequals(tokens[x], std::string("Auto")) == 1))
							{
								sql_output.push_back(2);
							}
							else
							{
								status = false;
								#ifdef TESTING
									std::cout << "extDB: DB_CUSTOM_V4: Bad Output Option: " << call_name << ":" << tokens[x] << std::endl;
								#endif
								BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V4: Bad Output Option " << call_name << ":" << tokens[x];
							}
						}
						custom_protocol[call_name].sql_output = sql_output;



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

								std::string input_quotes_stringval_str = "$INPUT_ADD_QUOTES_STRING_" + Poco::NumberFormatter::format(x);
								size_t input_quotes_stringval_len = input_stringval_str.length();

								std::string input_quotes_beguidval_str = "$INPUT_ADD_QUOTES_BEGUID_" + Poco::NumberFormatter::format(x);
								size_t input_quotes_beguidval_len = input_beguidval_str.length();

								for(std::list<Poco::DynamicAny>::iterator it_sql_list = sql_list.begin(); it_sql_list != sql_list.end(); ++it_sql_list) 
								{
									if (it_sql_list->isString())
									{
										std::string work_str = *it_sql_list;
										std::string tmp_str;
										while (true)
										{
											size_t pos = work_str.find(input_val_str);

											size_t pos_input_string = work_str.find(input_stringval_str);
											size_t pos_input_beguid = work_str.find(input_beguidval_str);

											size_t pos_input_quotes_string = work_str.find(input_quotes_stringval_str);
											size_t pos_input_quotes_beguid = work_str.find(input_quotes_beguidval_str);

											size_t min_pos = min(pos, min(min(pos_input_string, pos_input_beguid), min(pos_input_quotes_string, pos_input_quotes_beguid)));

											tmp_str = work_str.substr(0, min_pos);
											work_str = work_str.substr((min_pos + input_val_len), std::string::npos);

											if ((min_pos == pos) && (min_pos == pos_input_string)) // No More Found  n_pos
											{
												if (!work_str.empty())
												{
													sql_list.insert(it_sql_list, work_str);
													it_sql_list = sql_list.erase(it_sql_list);
													--it_sql_list;
												}
												break;
											}

											if (min_pos == pos)
											{
												std::list<Poco::DynamicAny>::iterator new_it_sql_vector = sql_list.insert(it_sql_list, x);
												new_it_sql_vector = sql_list.insert(new_it_sql_vector, tmp_str);
											}
											else if (min_pos == pos_input_string)
											{
												std::list<Poco::DynamicAny>::iterator new_it_sql_vector = sql_list.insert(it_sql_list, -x);
												new_it_sql_vector = sql_list.insert(new_it_sql_vector, tmp_str);
											}
											else if (min_pos == pos_input_quotes_string)
											{
												std::list<Poco::DynamicAny>::iterator new_it_sql_vector = sql_list.insert(it_sql_list, (-x - 1000));
												new_it_sql_vector = sql_list.insert(new_it_sql_vector, tmp_str);
											}
											else if (min_pos == pos_input_beguid)
											{
												std::list<Poco::DynamicAny>::iterator new_it_sql_vector = sql_list.insert(it_sql_list, (-x - 2000));
												new_it_sql_vector = sql_list.insert(new_it_sql_vector, tmp_str);
											}
											else if (min_pos == pos_input_quotes_beguid)
											{
													std::list<Poco::DynamicAny>::iterator new_it_sql_vector = sql_list.insert(it_sql_list, (-x -3000));
													new_it_sql_vector = sql_list.insert(new_it_sql_vector, tmp_str);
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
								std::cout << "extDB: DB_CUSTOM_V4: Unknown Bad Strings Action for " << call_name << ":" << custom_protocol[call_name].bad_chars_action << std::endl;
							#endif
							BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V4: Unknown Bad Strings Action for " << call_name << ":" << custom_protocol[call_name].bad_chars_action;
						}
					}
				}
			}
			else
			{
				status = false;
				#ifdef TESTING
					std::cout << "extDB: DB_CUSTOM_V4: Template File Missing Incompatiable Version" << db_template_file << std::endl;
				#endif
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V4: Template File Missing Incompatiable Version: " << db_template_file;
			}
		}
		else
		{
			status = false;
			#ifdef TESTING
				std::cout << "extDB: DB_CUSTOM_V4: Template File Missing Default Options" << db_template_file << std::endl;
			#endif
			BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V4: Template File Missing Default Options: " << db_template_file;
		}
	}
	else 
	{
		status = false;
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V4: Template File Not Found" << db_template_file << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_CUSTOM_V4: No Template File Found: " << db_template_file;
	}
	return status;
}


void DB_CUSTOM_V4::getBEGUID(std::string &input_str, std::string &result)
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


void DB_CUSTOM_V4::callCustomProtocol(AbstractExt *extension, boost::unordered_map<std::string, Template_Calls>::const_iterator itr, std::vector< std::string > &tokens, bool &sanitize_value_check_ok, std::string &result)
{
	Poco::Data::Session db_session = extension->getDBSession_mutexlock();
	Poco::Data::Statement sql_current(db_session);

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
				if (*it_sql_list > 0) // Convert $INPUT to String
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
				else if (*it_sql_list > - 1000) // Convert $INPUT to String ""
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
				else if (*it_sql_list > - 2000) // Convert $INPUT to BEGUID
				{
					std::string input_str_test = tokens[((-1 * *it_sql_list) - 2000)];
					getBEGUID(input_str_test, tmp_str);
					sql_str += tmp_str;
				}
				else // Convert $INPUT to "BEGUID"
				{
					std::string input_str_test = tokens[((-1 * *it_sql_list) - 3000)];
					getBEGUID(input_str_test, tmp_str);
					sql_str += "\"" + tmp_str + "\"";;
				}
			}
		}

		if (sanitize_value_check_ok)
		{
			try 
			{
				Poco::Data::Statement sql(db_session);
				sql << sql_str;
				sql.execute();
				sql_current.swap(sql);
			}
			catch (Poco::Data::SQLite::DBLockedException& e)
			{
				sanitize_value_check_ok = false;
				#ifdef TESTING
					std::cout << "extDB: DB_CUSTOM_V4: Error DBLockedException: " + e.displayText() << std::endl;
				#endif
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error DBLockedException: " + e.displayText();
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error DBLockedException: SQL:" + sql_str;
				result = "[0,\"Error DBLocked Exception\"]";
			}
			catch (Poco::Data::MySQL::ConnectionException& e)
			{
				sanitize_value_check_ok = false;
				#ifdef TESTING
					std::cout << "extDB: DB_CUSTOM_V4: Error ConnectionException: " + e.displayText() << std::endl;
				#endif
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error ConnectionException: " + e.displayText();
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error ConnectionException: SQL:" + sql_str;
			}
			catch(Poco::Data::MySQL::StatementException& e)
			{
				sanitize_value_check_ok = false;
				#ifdef TESTING
					std::cout << "extDB: DB_CUSTOM_V4: Error StatementException: " + e.displayText() << std::endl;
				#endif
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error StatementException: " + e.displayText();
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error StatementException: SQL:" + sql_str;
				result = "[0,\"Error Statement Exception\"]";
			}
			catch (Poco::Data::DataException& e)
			{
				sanitize_value_check_ok = false;
				#ifdef TESTING
					std::cout << "extDB: DB_CUSTOM_V4: Error DataException: " + e.displayText() << std::endl;
				#endif
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error DataException: " + e.displayText();
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error DataException: SQL:" + sql_str;
				result = "[0,\"Error Data Exception\"]";
			}
			catch (Poco::Exception& e)
			{
				sanitize_value_check_ok = false;
				#ifdef TESTING
					std::cout << "extDB: DB_CUSTOM_V4: Error Exception: " + e.displayText() << std::endl;
				#endif
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error Exception: " + e.displayText();
				BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error Exception: SQL:" + sql_str;
				result = "[0,\"Error Exception\"]";
			}
		}
		else
		{
			sanitize_value_check_ok = false;
			break;
		}
	}

	if (sanitize_value_check_ok)
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
				std::size_t sql_output_count = itr->second.sql_output.size();

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
						switch (itr->second.sql_output[col])
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
		#ifdef TESTING
			std::cout << "extDB: DB_CUSTOM_V4: Trace: Result: " + result << std::endl;
		#endif
		#ifdef DEBUG_LOGGING
			BOOST_LOG_SEV(extension->logger, boost::log::trivial::trace) << "extDB: DB_CUSTOM_V4: Trace: Result: " + result;
		#endif
	}
}


void DB_CUSTOM_V4::callProtocol(AbstractExt *extension, std::string input_str, std::string &result)
{
	#ifdef TESTING
		std::cout << "extDB: DB_CUSTOM_V4: Trace: " + input_str << std::endl;
	#endif
	#ifdef DEBUG_LOGGING
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::trace) << "extDB: DB_CUSTOM_V4: Trace: Input:" + input_str;
	#endif

	Poco::StringTokenizer tokens(input_str, ":");
	
	int token_count = tokens.count();
	boost::unordered_map<std::string, Template_Calls>::const_iterator itr = custom_protocol.find(tokens[0]);
	if (itr == custom_protocol.end())
	{
		result = "[0,\"Error No Custom Call Not Found\"]";
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error No Custom Call Not Found: " + input_str;
	}
	else
	{
		if (itr->second.number_of_inputs != (token_count - 1))
		{
			result = "[0,\"Error Incorrect Number of Inputs\"]";
			BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Incorrect Number of Inputs: " + input_str;
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
						BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error Bad Char Detected: Input:" + input_str;
						BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error Bad Char Detected: Token:" + tokens[i];
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
						BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error Bad Char Detected: Input:" + input_str;
						BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Error Bad Char Detected: Token:" + tokens[i];
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
					BOOST_LOG_SEV(extension->logger, boost::log::trivial::warning) << "extDB: DB_CUSTOM_V4: Sanitize Check error: Input:" + input_str;
				}
			}
			else
			{
				result = "[0,\"Error Bad Char Found\"]";
			}
		}
	}
}