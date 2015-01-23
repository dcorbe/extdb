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

#include <Poco/Data/MetaColumn.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>

#include <Poco/DateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeParser.h>

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Path.h>
#include <Poco/URI.h>

#include <Poco/Exception.h>

#include <Poco/NumberParser.h>

#include <Poco/Types.h>
#include <Poco/MD5Engine.h>
#include <Poco/DigestEngine.h>

#include <string>

#include "../sanitize.h"


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


std::string VAC::convertSteamIDtoBEGUID(const std::string &input_str)
// From Frank https://gist.github.com/Fank/11127158
// Modified to use libpoco
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

	boost::lock_guard<boost::mutex> lock(mutex_md5);
	md5.update(bestring.str());
	return Poco::DigestEngine::digestToHex(md5.digest());
}



void VAC::callProtocol(std::string input_str, std::string &result)
{
	Poco::StringTokenizer t_arg(input_str, ":");

	if (t_arg.count() == 2)
	{
		if (!isNumber(t_arg[1])) // Check Valid Steam ID
		{
			result = "[0, \"VAC: Error Invalid Steam ID\"]";
		}
		else
		{
			if (boost::iequals(t_arg[0], std::string("GetFriends")) == 1)
			{
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
					//result = "[1," + Poco::NumberFormatter::format(VACBans_Cache->get(t_arg[1])->NumberOfVACBans) + "]";
				}
				else if (boost::iequals(t_arg[0], std::string("DaysSinceLastBan")) == 1)
				{
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


bool VAC::init(AbstractExt *extension, AbstractExt::DBConnectionInfo *database, const std::string init_str) 
{
	extension_ptr = extension;
	//database_ptr = database;

	vac_api_key = extension_ptr->pConf->getString("Main.Steam Web API Key", "");
	vac_ban_check.NumberOfVACBans = extension_ptr->pConf->getInt("VAC.NumberOfVACBans", 1);
	vac_ban_check.DaysSinceLastBan = extension_ptr->pConf->getInt("VAC.DaysSinceLastBan", 0);
	vac_ban_check.BanDuration = extension_ptr->pConf->getString("VAC.BanDuration", "0");
	vac_ban_check.BanMessage = extension_ptr->pConf->getString("VAC.BanMessage", "Steam VAC Banned");

	VACBans_Cache = new Poco::ExpireCache<std::string, SteamVacBans>(extension_ptr->pConf->getInt("VAC.BanCacheTime", 3600000));
	VACFriends_Cache = new Poco::ExpireCache<std::string, SteamVacFriends>(extension_ptr->pConf->getInt("VAC.FriendsCacheTime", 3600000));

	return true;
}