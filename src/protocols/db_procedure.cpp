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


#include "db_procedure_v2.h"

#include <Poco/Data/Common.h>
#include <Poco/Data/MetaColumn.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>
#include <Poco/Exception.h>
#include <Poco/StringTokenizer.h>

#include "Poco/Data/MySQL/Connector.h"
#include "Poco/Data/MySQL/MySQLException.h"
#include "Poco/Data/ODBC/Connector.h"
#include "Poco/Data/ODBC/ODBCException.h"

#include <cstdlib>
#include <iostream>

#include "../sanitize.h"


bool DB_PROCEDURE_V2::init(AbstractExt *extension, std::string init_str)
{
	pLogger = &Poco::Logger::get("DB_PROCEDURE_V2");
	
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
		// DATABASE NOT SETUP YET or SQLITE Doesn't Support Procedures
		#ifdef TESTING
			std::cout << "extDB: DB_PROCEDURE: Doesn't Support SQLite" << std::endl;
		#endif
		pLogger->warning("Doesn't Support SQLite");
		return false;
	}
	else
	{
		// DATABASE NOT SETUP YET or SQLITE Doesn't Support Procedures
		#ifdef TESTING
			std::cout << "extDB: DB_PROCEDURE: No Database Connection" << std::endl;
		#endif
		pLogger->warning("No Database Connection");
		return false;
	}
}

bool DB_PROCEDURE_V2::isNumber(const std::string &input_str)
{
bool status = true;
	for (unsigned int index=0; index < input_str.length(); index++)
	{
		if (!isdigit(input_str[index]))
		{
			status = false;
			break;
		}
	}
return status;
}

void DB_PROCEDURE_V2::callProtocol(AbstractExt *extension, std::string input_str, std::string &result)
{
//  Unique ID
//   |
//  Procedure Name
//   |
//  Input
//   |
//  Output Count
	#ifdef TESTING
		std::cout << "extDB: DEBUG INFO: " + input_str << std::endl;
	#endif
	#ifdef DEBUG_LOGGING
		pLogger->trace(" " + input_str);
	#endif

    try
    {
		Poco::StringTokenizer t_arg(input_str, "|");
		const int num_of_inputs = t_arg.count();
		if ((num_of_inputs == 4) && (t_arg[1].length() >= 3) && (isNumber(t_arg[0])))
		{
			std::string sql_str_procedure = "call " + t_arg[1].substr(1, (t_arg[1].length() - 2)) + "(";
			std::string sql_str_select = "SELECT ";

			if ( (Sqf::check(t_arg[0])) && (Sqf::check(t_arg[1])) )
			{
				// Inputs
				bool sanitize_check = true;
				Poco::StringTokenizer t_arg_inputs(t_arg[2], ":");
				const int num_of_inputs = t_arg_inputs.count();
				for(int i = 0; i != num_of_inputs; ++i) {
					if (!Sqf::check(t_arg_inputs[i]))
					{
						sanitize_check = false;
						break;
					}
					sql_str_procedure += t_arg_inputs[i] + ", ";
				}
				
				if (sanitize_check)
				{
					// Outputs
					const int num_of_outputs = Poco::NumberParser::parse(t_arg[3]);
					if (num_of_outputs <= 0)
					{
						if (num_of_inputs > 0)
						{
							// Remove the trailing ", " if no outputs
							sql_str_procedure = sql_str_procedure.substr(0,(sql_str_procedure.length() - 2));  
						}
						sql_str_procedure += ")";
					}
					else
					{
						// Generate Output Values
						unique_id = extension->getUniqueID_mutexlock(); // Using this to make sure no clashing of Output Values
						for(int i = 0; i != num_of_outputs; ++i) {
							const std::string temp_str = "@Output" + Poco::NumberFormatter::format(i) + "_" + Poco::NumberFormatter::format(unique_id) +  + "_" + t_arg[0] + ", ";
							sql_str_procedure += temp_str;
							sql_str_select += temp_str;
						}
						// Remove the trailing ", "
						sql_str_procedure = sql_str_procedure.substr(0,(sql_str_procedure.length() - 2));  // Remove the trailing ", " if there is outputs
						sql_str_select = sql_str_select.substr(0,(sql_str_select.length() - 2));
						sql_str_procedure += ")";
					}

					// SQL Call Statement
					Poco::Data::Session db_session = extension->getDBSession_mutexlock();
					Poco::Data::Statement sql(db_session);

					sql << sql_str_procedure, Poco::Data::now;  // TODO: See if can get any error message if unsuccessfull

					result = "[";

					if (num_of_outputs > 0)
					{
						// If Outputs.. SQL SELECT Statement to get Results
						
						Poco::Data::Statement sql2(db_session);
						sql2 << sql_str_select, Poco::Data::now;
							
						extension->freeUniqueID_mutexlock(unique_id); // Free Unique ID
							
						Poco::Data::RecordSet rs(sql2);
						
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
										result += "\"" + (rs[col].convert<std::string>() + "\"");
									}
									else
									{
										result += rs[col].convert<std::string>();
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
					}
					result += "]";
					#ifdef TESTING
						std::cout << "extDB: DEBUG INFO: RESULT:" + result << std::endl;
					#endif
					#ifdef DEBUG_LOGGING
						pLogger->trace(" RESULT:" + result);
					#endif
				}
				else
				{
					result = "[0,\"Invalid Value for Unique Server ID / Procedure Name \"]";
				}
			}
			else
			{
				// Sanitize Check for Procedure Name + Unique ID for Serveer
				result = "[0,\"Invalid Value for Unique Server ID / Procedure Name \"]";
			}
		}
		else
		{
			#ifdef TESTING
				std::cout << "extDB: DEBUG INFO: Invalid Format" << std::endl;
			#endif
			pLogger->error(" Invalid Format: " + input_str);
			result = "[0,\"Invalid Format\"]";
		}
	}
    catch (Poco::Exception& e)
    {
		#ifdef TESTING
			std::cout << "extdb: Error: " << e.displayText() << std::endl;
		#endif 
		pLogger->critical("Input: " + input_str);
		pLogger->critical("Exception: " + e.displayText());
		result = "[0,\"Error Exception\"]";
    }
}