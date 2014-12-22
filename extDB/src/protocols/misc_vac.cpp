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


#include "misc_vac.h"

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


bool MISC_VAC::init(AbstractExt *extension, std::string init_str) 
{
	vac_ban_check.NumberOfVACBans = extension->pConf->getInt("VAC.NumberOfVACBans", 1);
	vac_ban_check.DaysSinceLastBan = extension->pConf->getInt("VAC.DaysSinceLastBan", 0);
	vac_ban_check.BanDuration = extension->pConf->getString("VAC.BanDuration", "0");
	vac_ban_check.BanMessage = extension->pConf->getString("VAC.BanMessage", "VAC BAN FOUND");

	VAC_Cache = (new Poco::ExpireCache<std::string, SteamVacInfo>(3600000));

	return true;
}


bool MISC_VAC::isNumber(const std::string &input_str)
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


std::string MISC_VAC::convertSteamIDtoBEGUID(const std::string &input_str)
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

	md5.update(bestring.str());
	return Poco::DigestEngine::digestToHex(md5.digest());
}


bool MISC_VAC::updateVAC(std::string steam_web_api_key, std::string &steam_id)
{
	if (!(VAC_Cache->has(steam_id)))
	{
		SteamVacInfo vac_info;

		Poco::URI uri("http://api.steampowered.com/ISteamUser/GetPlayerBans/v1/?key=" + steam_web_api_key + "&format=json&steamids=" + steam_id);
		Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());

		// prepare path
		std::string path(uri.getPathAndQuery());
		if (path.empty()) path = "/";

		// send request
		Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
		session.sendRequest(req);

		// get response
		Poco::Net::HTTPResponse res;
		//http://www.appinf.com/docs/poco/Poco.Net.HTTPResponse.html#17176
		#ifdef TESTING
			std::cout << res.getStatus() << " " << res.getReason() << std::endl;
		#endif

		// print response
		std::istream &is = session.receiveResponse(res);

		boost::property_tree::ptree pt;
		boost::property_tree::read_json(is, pt);

		vac_info.SteamID = pt.get<std::string>("players..SteamId", "FAILED");
		vac_info.NumberOfVACBans = pt.get<std::string>("players..NumberOfVACBans", "FAILED");
		vac_info.VACBanned = pt.get<std::string>("players..VACBanned", "FAILED");
		vac_info.DaysSinceLastBan = pt.get<std::string>("players..DaysSinceLastBan", "FAILED");
		vac_info.EconomyBan = pt.get<std::string>("players..EconomyBan", "FAILED");

		if ((vac_info.SteamID == "FAILED") || (vac_info.NumberOfVACBans == "FAILED")|| (vac_info.VACBanned == "FAILED") || (vac_info.DaysSinceLastBan == "FAILED") || (vac_info.EconomyBan == "FAILED"))
		{
			return false;
		}
		else
		{
			boost::lock_guard<boost::mutex> lock(VAC_Cache_mutex);
			VAC_Cache.add(steam_id, std::move(vac_info)); // Update Cache
		}
	}
	return true;
}


bool DB_VAC::getCache(std::string &steam_id)
{

}


void MISC_VAC::callProtocol(AbstractExt *extension, std::string input_str, std::string &result)
{
	Poco::StringTokenizer t_arg(input_str, ":");
	const int num_of_inputs = t_arg.count();
	if (num_of_inputs == 2)
	{
		if (!isNumber(t_arg[1])) // Check Valid Steam ID
		{
			result  = "[0, \"MISC_VAC: Error Invalid Steam ID\"]";
		}
		else
		{
			std::string steamdID = convertSteamIDtoBEGUID(t_arg[1]);
			updateVAC(extension->getAPIKey(), steamdID);

			if (boost::iequals(t_arg[0], std::string("GetFriends")) == 1)
			{
				//TODO
				result  = "[0, \"MISC_VAC: NOT WORKING YET\"]";
			}
			else if (boost::iequals(t_arg[0], std::string("VACBanned")) == 1)
			{
				//TODO
				result  = "[0, \"MISC_VAC: NOT WORKING YET\"]";
			}
			else if (boost::iequals(t_arg[0], std::string("NumberOfVACBans")) == 1)
			{
				//TODO
				result  = "[0, \"MISC_VAC: NOT WORKING YET\"]";
			}
			else if (boost::iequals(t_arg[0], std::string("DaysSinceLastBan")) == 1)
			{
				//TODO
				result  = "[0, \"MISC_VAC: NOT WORKING YET\"]";
			}
			else if (boost::iequals(t_arg[0], std::string("EconomyBan")) == 1)
			{
				//TODO
				result  = "[0, \"MISC_VAC: NOT WORKING YET\"]";
			}
			else if (boost::iequals(t_arg[0], std::string("VACAutoBan")) == 1)
			{
				if (true)
				{
					//if ((Poco::NumberParser::parse(vac_info.NumberOfVACBans) >= vac_ban_check.NumberOfVACBans) && (Poco::NumberParser::parse(vac_info.DaysSinceLastBan) <=vac_ban_check.DaysSinceLastBan ))
						//rcon.sendCommand("ban " + convertSteamIDtoBEGUID(steam_id) + " " + vac_ban_check.BanDuration + " " + vac_ban_check.BanMessage);
				}
			}
			else
			{
				result  = "[0, \"MISC_VAC: Invalid Query Type\"]";
			}
		}
	}
}