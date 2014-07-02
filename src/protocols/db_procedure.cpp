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



#include "db_procedure.h"

#include <Poco/Data/MetaColumn.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>
#include <Poco/Exception.h>
#include <Poco/StringTokenizer.h>

#include <cstdlib>
#include <iostream>

using namespace Poco::Data::Keywords;


std::string DB_PROCEDURE::callProtocol(AbstractExt *extension, std::string input_str)
{
//  Procedure Name
//   ::
//  Input
//   ::
//  Output Count

    try
    {
		std::cout << "DEBUG: " << input_str << std::endl;
		std::string::size_type found = input_str.find("::");
		if (found==std::string::npos)  // Check Invalid Format
		{
			return "[0,\"Error Invalid Format\"]";
		}
		else
		{
			std::string procedure_name = input_str.substr(0, found);
			input_str = input_str.substr(found + 2);
			std::cout << "DEBUG: " << input_str << std::endl;
			found = input_str.find("::");
			if (found==std::string::npos)  // Check Invalid Format
			{
				return "[0,\"Error Invalid Format\"]";
			}
			else
			{
				std::string arg_inputs = input_str.substr(0, found);
				input_str = input_str.substr(found + 2);
				std::cout << "DEBUG: " << input_str << std::endl;

				// Sanitize Prodecure Name
				std::string temp_str;
				std::string sql_str_procedure = "call " + procedure_name + "(";
				std::string sql_str_select = "SELECT ";
				
				// Inputs
				Poco::StringTokenizer t_arg_inputs(arg_inputs, ":");
				const int num_of_inputs = t_arg_inputs.count();

				for(int i = 0; i != num_of_inputs; ++i) {
					sql_str_procedure += t_arg_inputs[0] + ", ";
				}
				
				// Outputs
				const int num_of_outputs = Poco::NumberParser::parse(input_str);
				if ((num_of_inputs > 0) && (num_of_outputs > 0))
				{
					sql_str_procedure = sql_str_procedure.substr(0,(sql_str_procedure.length() - 2));  // Remove the trailing ", " if no outputs
				}
				
				//		Generate Output Values
				const int unique_id = extension->getUniqueID_mutexlock(); // Using this to make sure no clashing of Output Values
				std::string unique_id_str = Poco::NumberFormatter::format(unique_id);
				
				for(int i = 0; i != num_of_outputs; ++i) {
					temp_str = "@Output" + Poco::NumberFormatter::format(i) + "_" + unique_id_str;
					sql_str_procedure += "@Output" + Poco::NumberFormatter::format(i) + "_" + unique_id_str + ", ";
					sql_str_select += temp_str + ", ";
				}
				
				if (num_of_outputs > 0)
				{
					sql_str_procedure = sql_str_procedure.substr(0,(sql_str_procedure.length() - 2));  // Remove the trailing ", " if there is outputs
					sql_str_select = sql_str_select.substr(0,(sql_str_select.length() - 2));
				}
				
				sql_str_procedure += ")";
				
				Poco::Data::Session db_session = extension->getDBSession_mutexlock();
				Poco::Data::Statement sql(db_session);

				sql << sql_str_procedure, now;  // TODO: See if can get any error message if unsuccessfull
				
				Poco::Data::Statement sql2(db_session);
				sql2 << sql_str_select, now;
				
				extension->freeUniqueID_mutexlock(unique_id); // No longer need id
				
				Poco::Data::RecordSet rs(sql2);
				
				std::string result;
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
				result += "]";
				return result;
			}
		}
	}
    catch (Poco::Exception& e)
    {
		#ifdef TESTING
			std::cout << "extdb: Error: " << e.displayText() << std::endl;
		#endif 
		#ifdef LOGGING
			BOOST_LOG_SEV(logger, boost::log::trivial::fatal) << " DB_PROCEDURE: Error: " + e.displayText();
		#endif
        return "[0,\"Error\"]";
    }
}