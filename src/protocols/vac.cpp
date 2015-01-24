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

Code to Convert SteamID -> BEGUID 
From Frank https://gist.github.com/Fank/11127158

*/


#include "vac.h"

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <Poco/StringTokenizer.h>

#include <string>

#include "../steam.h"


bool VAC::init(AbstractExt *extension, AbstractExt::DBConnectionInfo *database, const std::string init_str) 
{
	extension_ptr = extension;
	//database_ptr = database;
	return true;
}


bool VAC::isNumber(const std::string &input_str)
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


bool VAC::callProtocol(std::string input_str, std::string &result, const int unique_id)
{
	#ifdef TESTING
		extension_ptr->console->info("extDB: VAC: Trace: Input: {0}", input_str);
	#endif
	#ifdef DEBUG_LOGGING
		extension_ptr->logger->info("extDB: VAC: Trace: Input: {0}", input_str);
	#endif

	if (unique_id == -1)
	{
		#ifdef TESTING
			extension_ptr->console->warn("extDB: VAC: SYNC MODE NOT SUPPORTED");
		#endif
		extension_ptr->logger->warn("extDB: VAC: SYNC MODE NOT SUPPORTED");
		result = "[0, \"VAC: SYNC MODE NOT SUPPORTED\"]";
	}
	else
	{
		Poco::StringTokenizer t_arg(input_str, ":");
		result.clear();

		if (t_arg.count() > 1)
		{
			bool status = true;
			std::vector<std::string> steamIDs;
			for (int i = 1; i < t_arg.count(); ++i)
			{
				if (isNumber(t_arg[i]))
				{
					steamIDs.push_back(t_arg[i]);
				}
				else
				{
					#ifdef TESTING
						extension_ptr->console->warn("extDB: VAC: Invalid SteamID: {0}", t_arg[i]);
					#endif
					extension_ptr->logger->warn("extDB: VAC: Invalid SteamID: {0}", t_arg[i]);
					result = "[0, \"VAC: Invalid SteamID\"]";
					status = false;
					break;
				}
			}

			if (status)
			{
				if (boost::iequals(t_arg[0], std::string("GetFriends")) == 1)
				{
					extension_ptr->steamQuery(unique_id, true, false, steamIDs, true);
				}
				else if (boost::iequals(t_arg[0], std::string("VACBanned")) == 1)
				{
					extension_ptr->steamQuery(unique_id, false, true, steamIDs, true);
				}
				else
				{
					#ifdef TESTING
						extension_ptr->console->warn("extDB: VAC: Invalid Query Type: {0}", t_arg[0]);
					#endif
					extension_ptr->logger->warn("extDB: VAC: Invalid Query Type: {0}", t_arg[0]);
					result = "[0, \"VAC: Invalid Query Type\"]";
					status = false;

				}
			}
		}
	}
	
	if (result.empty())
	{
		return false; //Override Saving Result
	}
	else
	{
		return true;
	}
}