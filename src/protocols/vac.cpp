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
	if (unique_id == -1)
	{
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
			for (int i = 1; i < t_arg.count() - 1; ++i)
			{
				if (isNumber(t_arg[i]))
				{
					steamIDs.push_back(t_arg[i]);
				}
				else
				{
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
					result = "[0, \"VAC: NOT WORKING YET\"]";
				}
				else
				{
					if (boost::iequals(t_arg[0], std::string("VACBanned")) == 1)
					{
						result = "[0, \"VAC: NOT WORKING YET\"]";
	//					if (VACBans_Cache->get(t_arg[1])->VACBanned)
	//					{
	//						result = "[1,1]";
	//					}
	//					else
	//					{
	//						result = "[1,0]";
	//					}
					}
					else if (boost::iequals(t_arg[0], std::string("NumberOfVACBans")) == 1)
					{
						result = "[0, \"VAC: NOT WORKING YET\"]";
						//result = "[1," + Poco::NumberFormatter::format(VACBans_Cache->get(t_arg[1])->NumberOfVACBans) + "]";
					}
					else if (boost::iequals(t_arg[0], std::string("DaysSinceLastBan")) == 1)
					{
						result = "[0, \"VAC: NOT WORKING YET\"]";
						//result = "[1," + Poco::NumberFormatter::format(VACBans_Cache->get(t_arg[1])->DaysSinceLastBan) + "]";
					}
					else
					{
						result = "[0, \"VAC: Invalid Query Type\"]";
					}
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


bool VAC::init(AbstractExt *extension, AbstractExt::DBConnectionInfo *database, const std::string init_str) 
{
	extension_ptr = extension;
	//database_ptr = database;
	return true;
}