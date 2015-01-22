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


bool VAC::convertSteamIDtoBEGUID(const std::string &input_str, std::string &result)
// From Frank https://gist.github.com/Fank/11127158
// Modified to use libpoco
{
	bool status = true;
	if (input_str.empty())
	{
		status = false;
	}
	else
	{
		for (unsigned int index=0; index < input_str.length(); index++)
		{
			if (!std::isdigit(input_str[index]))
			{
				status = false;
				break;
			}
		}
	}

	if (!status)
	{
		result = "[0,\"Invalid SteamID\"";
	}
	else
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
		result = Poco::DigestEngine::digestToHex(md5.digest());
	}
	return status;
}


bool VAC::updateVAC(std::string steam_web_api_key, std::string &steamID)
{
	if (!(VACBans_Cache->has(steamID)))
	{
		SteamVacBans vac_info;

		std::string url = "http://api.steampowered.com/ISteamUser/GetPlayerBans/v1/?key=" + steam_web_api_key + "&format=json&steamids=" + steamID;
		extension_ptr->console->info("{0}", url);
		Poco::URI uri(url);
		Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());

		// prepare path
		std::string path(uri.getPathAndQuery());
		if (path.empty()) path = "/";

		// send request
		Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
		session.sendRequest(req);

		// get response
		Poco::Net::HTTPResponse res;
		#ifdef TESTING
			extension_ptr->console->info("VAC: Response Status {0}", res.getReason());
			extension_ptr->console->info("VAC: Response Status {0}", res.getStatus());
		#endif

		if (res.getStatus() == 200)
		{
			try
			{
				//http://www.appinf.com/docs/poco/Poco.Net.HTTPResponse.html#17176

				std::istream &is = session.receiveResponse(res);
				//std::cout << is.rdbuf() << std::endl;

				boost::property_tree::ptree pt;
				boost::property_tree::read_json(is, pt);

				std::cout << "-----------------------" << std::endl;
				//std::cout <<  pt.get<int>("players..NumberOfVACBans", 0) << std::endl;
				//std::cout <<  pt.get<bool>("players..VACBanned", false) << std::endl;
				//std::cout <<  pt.get<int>("players..DaysSinceLastBan", 0) << std::endl;

				vac_info.steamID = pt.get<std::string>("players..SteamId", "");
				std::cout <<  pt.get<std::string>("players..SteamId", "") << std::endl;;
				std::cout << vac_info.steamID << std::endl;
				vac_info.NumberOfVACBans = pt.get<int>("players..NumberOfVACBans", 0);
				vac_info.VACBanned = pt.get<bool>("players..VACBanned", false);
				vac_info.DaysSinceLastBan = pt.get<int>("players..DaysSinceLastBan", 0);

				if (vac_info.steamID.empty())
				{
					std::cout << "STEAM ID EMPTY" << std::endl;
					return false;
				}
				else if (vac_info.steamID == steamID)
				{
					std::cout << "STEAM ID EXISTS" << std::endl;
					VACBans_Cache->add(steamID, std::move(vac_info)); // Update Cache
				}
				else
				{
					std::cout << "STEAM ID NOPE" << std::endl;
				}
			}
			catch(boost::property_tree::json_parser::json_parser_error &je)
			{
				std::cout << "Error parsing: " << je.filename() << " on line: " << je.line() << std::endl;
				std::cout << je.message() << std::endl;
			}
		}
		else
		{
			extension_ptr->logger->info("Steam VAC Error (Service Down?): SteamID {0}: Response Status {1}", steamID, res.getReason());
			extension_ptr->console->info("Steam VAC Error (Service Down?): SteamID {0}: Response Status {1}", steamID, res.getReason());
			return false;
		}
	}
	return true;
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
				if (true) // TODO ADD Check Player on Server
				{
					if (updateVAC(extension_ptr->getAPIKey(), t_arg[1]))
					{
						if (boost::iequals(t_arg[0], std::string("VACBanned")) == 1)
						{
							if (VACBans_Cache->get(t_arg[1])->VACBanned)
							{
								result = "[1,1]";
							}
							else
							{
								result = "[1,0]";
							}
						}
						else if (boost::iequals(t_arg[0], std::string("NumberOfVACBans")) == 1)
						{
							result = "[1," + Poco::NumberFormatter::format(VACBans_Cache->get(t_arg[1])->NumberOfVACBans) + "]";
						}
						else if (boost::iequals(t_arg[0], std::string("DaysSinceLastBan")) == 1)
						{
							result = "[1," + Poco::NumberFormatter::format(VACBans_Cache->get(t_arg[1])->DaysSinceLastBan) + "]";
						}
						else if (boost::iequals(t_arg[0], std::string("VACAutoBan")) == 1)
						{
							if (true)
							{
								if ((VACBans_Cache->get(t_arg[1])->NumberOfVACBans >= vac_ban_check.NumberOfVACBans) && (VACBans_Cache->get(t_arg[1])->DaysSinceLastBan <= vac_ban_check.DaysSinceLastBan))
								{
									result = "[1,1]";
									//rcon.sendCommand("ban " + convertSteamIDtoBEGUID(t_arg[1]) + " " + vac_ban_check.BanDuration + " " + vac_ban_check.BanMessage);
								}
								else
								{
									result = "[1,0]";
								}
							}
						}
						else
						{
							result = "[0, \"VAC: Invalid Query Type\"]";
						}
					}
					else
					{
						result = "[0, \"VAC: Steam Query Error\"]";
					}
				}
				else
				{
					result = "[0, \"VAC: Invalid SteamID\"]";
				}
			}
		}
	}
}


bool VAC::init(AbstractExt *extension, AbstractExt::DBConnectionInfo *database, const std::string init_str) 
{
	extension_ptr = extension;
	//database_ptr = database;

	vac_ban_check.NumberOfVACBans = extension_ptr->pConf->getInt("VAC.NumberOfVACBans", 1);
	vac_ban_check.DaysSinceLastBan = extension_ptr->pConf->getInt("VAC.DaysSinceLastBan", 0);
	vac_ban_check.BanDuration = extension_ptr->pConf->getString("VAC.BanDuration", "0");
	vac_ban_check.BanMessage = extension_ptr->pConf->getString("VAC.BanMessage", "Steam VAC Banned");

	VACBans_Cache = new Poco::ExpireCache<std::string, SteamVacBans>(extension_ptr->pConf->getInt("VAC.BanCacheTime", 3600000));
	VACFriends_Cache = new Poco::ExpireCache<std::string, SteamVacFriends>(extension_ptr->pConf->getInt("VAC.FriendsCacheTime", 3600000));

	return true;
}