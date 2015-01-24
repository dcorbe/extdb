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


#include "steam.h"

#include <boost/algorithm/string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include <Poco/MD5Engine.h>
#include <Poco/DigestEngine.h>

#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/StreamCopier.h>
#include <Poco/Path.h>
#include <Poco/URI.h>

#include <Poco/NumberParser.h>

#include <Poco/Thread.h>
#include <Poco/Types.h>

#include <Poco/Exception.h>

#include <string>


std::string STEAM::convertSteamIDtoBEGUID(const std::string &input_str)
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


std::vector<std::string> STEAM::generateSteamIDStrings(std::vector<std::string> &steamIDs)
// Steam Only Allows 100 SteamIDs at a time
{
	std::string steamIDs_str;
	std::vector<std::string> steamIDs_list;
	
	int counter = 0;
	for (auto &val: steamIDs)
	{
		if (counter = 100)
		{
			steamIDs_str.erase(steamIDs_str.begin());
			steamIDs_str.pop_back();
			steamIDs_list.push_back(steamIDs_str);
			
			steamIDs_str.clear();
			counter = 0;
		}
		++counter;
		steamIDs_str += steamIDs_str + ",";
	}
	if (!steamIDs_str.empty())
	{
		steamIDs_str.erase(steamIDs_str.begin());
		steamIDs_str.pop_back();
		steamIDs_list.push_back(steamIDs_str);
	}
	return steamIDs_list;
}


void STEAM::updateSTEAMBans(std::vector<std::string> &steamIDs)
{
	std::sort(steamIDs.begin(), steamIDs.end());
	auto last = std::unique(steamIDs.begin(), steamIDs.end());
	steamIDs.erase(last, steamIDs.end());

	std::vector<std::string> update_steamIDs;
	for (auto &steamID: steamIDs)
	{
		if (!(SteamVacBans_Cache->has(steamID)))
		{
			update_steamIDs.push_back(steamID);
		}
	}

	std::vector<std::string> steamIDStrings = generateSteamIDStrings(update_steamIDs);
	for (auto &steamIDString: steamIDStrings)
	{
		std::string url = "http://api.steampowered.com/ISteamUser/GetPlayerBans/v1/?key=" + STEAM_api_key + "&format=json&steamids=" + steamIDString;	
		extension_ptr->console->info("{0}", url);
		extension_ptr->logger->info("{0}", url);

		Poco::URI uri(url);
		Poco::Net::HTTPClientSession session(uri.getHost(), uri.getPort());
		Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, (uri.getPathAndQuery()), Poco::Net::HTTPMessage::HTTP_1_1);
		session.sendRequest(req);
		Poco::Net::HTTPResponse res;
		extension_ptr->console->info("STEAM: Response Status {0}", res.getReason());
		extension_ptr->console->info("STEAM: Response Status {0}", res.getStatus());

		if (res.getStatus() == 200)
		{
			try
			{
				//http://www.appinf.com/docs/poco/Poco.Net.HTTPResponse.html#17176
				std::istream &is = session.receiveResponse(res);

				boost::property_tree::ptree pt;
				boost::property_tree::read_json(is, pt);
				
				for (const auto &val : pt.get_child("players"))
				{
					SteamVACBans steam_info;
					steam_info.steamID = val.second.get<std::string>("SteamId", "");
					steam_info.NumberOfVACBans = val.second.get<int>("NumberOfVACBans", 0);
					steam_info.VACBanned = val.second.get<bool>("VACBanned", false);
					steam_info.DaysSinceLastBan = val.second.get<int>("DaysSinceLastBan", 0);
					SteamVacBans_Cache->add(steam_info.steamID, steam_info);
					extension_ptr->console->info("STEAMID: {0}", steam_info.steamID);

					if ((steam_info.NumberOfVACBans >= rconBanSettings.NumberOfVACBans) && (steam_info.DaysSinceLastBan <= rconBanSettings.DaysSinceLastBan))
					{
						extension_ptr->rconCommand("ban " + convertSteamIDtoBEGUID(steam_info.steamID) + " " + rconBanSettings.BanDuration + " " + rconBanSettings.BanMessage);
					}
				}
			}
			catch (boost::property_tree::json_parser::json_parser_error &e)
			{
				extension_ptr->logger->error("Steam STEAM Error: Parsing Error: {0}", e.message());
				#ifdef TESTING
					extension_ptr->console->error("Steam STEAM Error: Parsing Error: {0}", e.message());
				#endif
			}
		}
		else
		{
			extension_ptr->logger->info("Steam STEAM Error (Service Down?): Response Status {0}", res.getReason());
			extension_ptr->console->info("Steam STEAM Error (Service Down?): Response Status {0}", res.getReason());
		}
	}
}


void STEAM::addQuery(const int &unique_id, bool queryFriends, bool queryVacBans, std::vector<std::string> &steamIDs)
{
	if (*steam_run_flag)
	{
		SteamQuery info;
		info.unique_id = unique_id;
		info.queryFriends = queryFriends;
		info.queryVACBans = queryVacBans;
		info.steamIDs = steamIDs;
		boost::lock_guard<boost::mutex> lock(mutex_query_queue);

		query_queue.push_back(std::move(info));
	}
	else
	{
		extension_ptr->saveResult_mutexlock(unique_id, "[0,\"extDB: Steam is not running / disabled\"]");
	}
}


void STEAM::run()
{
	*steam_run_flag = true;
	while (*steam_run_flag)
	{
		Poco::Thread::trySleep(60000); // 1 Minute Sleep unless woken up
		extension_ptr->console->info("Woke up Thread");

		std::vector<SteamQuery> query_queue_copy;
		{
			boost::lock_guard<boost::mutex> lock(mutex_query_queue);
			query_queue_copy = query_queue;
			query_queue.clear();
		}

		if (!query_queue_copy.empty())
		{
			std::vector<std::string> steamIDs_friends;
			std::vector<std::string> steamIDs_bans;

			for (auto &val: query_queue_copy)
			{
				if (val.queryFriends)
				{
					steamIDs_friends.insert(steamIDs_friends.end(), val.steamIDs.begin(), val.steamIDs.end());
				}
				if (val.queryVACBans)
				{
					steamIDs_bans.insert(steamIDs_bans.end(), val.steamIDs.begin(), val.steamIDs.end());
				}
			}

//			updateSTEAMFriends(steamIDs_friends);
			updateSTEAMBans(steamIDs_bans);

			std::string result;
			for (auto &val: query_queue_copy)
			{
				if (val.unique_id != -1)
				{
					if (val.queryFriends)
					{
						result.clear();
						for (auto &steamID: val.steamIDs)
						{
							//result =+ ;
						}						
						break;
					}
					if (val.queryVACBans)
					{
						result.clear();
						for (auto &steamID : val.steamIDs)
						{
							Poco::SharedPtr<SteamVACBans> info;
							info = SteamVacBans_Cache->get(steamID);
							if (info.isNull()) // Incase entry expired
							{
								result =+ ",false,";
							}
							else
							{
								if (info->extDBBanned)
								{
									result =+ ",true,";
								}
								else
								{
									result =+ ",false,";
								}
							}
						}
						break;
					}
					result.erase(result.begin());
					result.pop_back();
					extension_ptr->saveResult_mutexlock(val.unique_id, result);
				}
			}
		}
	}
}


void STEAM::stop()
{
	*steam_run_flag = false;
}


void STEAM::init(AbstractExt *extension)
{
	extension_ptr = extension;

	steam_run_flag = new std::atomic<bool>(false);

	STEAM_api_key = extension_ptr->pConf->getString("Steam.API Key", "");
	rconBanSettings.NumberOfVACBans = extension_ptr->pConf->getInt("STEAM.NumberOfVACBans", 1);
	rconBanSettings.DaysSinceLastBan = extension_ptr->pConf->getInt("STEAM.DaysSinceLastBan", 0);
	rconBanSettings.BanDuration = extension_ptr->pConf->getString("STEAM.BanDuration", "0");
	rconBanSettings.BanMessage = extension_ptr->pConf->getString("STEAM.BanMessage", "VAC Banned");

	SteamVacBans_Cache = new Poco::ExpireCache<std::string, SteamVACBans>(extension_ptr->pConf->getInt("STEAM.BanCacheTime", 3600000));
	SteamFriends_Cache = new Poco::ExpireCache<std::string, SteamFriends>(extension_ptr->pConf->getInt("STEAM.FriendsCacheTime", 3600000));
}
