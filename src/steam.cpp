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


// --------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------

void STEAMGET::init(AbstractExt *extension)
{
	extension_ptr = extension;

	session.setHost("api.steampowered.com");
	session.setPort(80);
	session.setTimeout(Poco::Timespan(30,0));
}


void STEAMGET::update(std::string &input_str)
{
	path = input_str;
}


int STEAMGET::getResponse(boost::property_tree::ptree &pt)
{
	#ifdef TESTING
		extension_ptr->console->info("{0}", path);
	#endif
	#ifdef DEBUG_LOGGING
		extension_ptr->logger->info("{0}", path);
	#endif

	Poco::Net::HTTPResponse response;
	if (response.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
	{
		try
		{
			std::istream &is = session.receiveResponse(response);
			boost::property_tree::read_json(is, pt);
			return 1;
		}
		catch (boost::property_tree::json_parser::json_parser_error &e)
		{
			#ifdef TESTING
				extension_ptr->console->error("Steam WEB API Error: Parsing Error Message: {0}, URI: {1}", e.message(), path);
			#endif
			extension_ptr->logger->error("Steam WEB API Error: Parsing Error Message: {0}, URI: {1}", e.message(), path);
			return -1;
		}
	}
	else
	{
		return 0;
	}
}


void STEAMGET::run()
{
	Poco::Net::HTTPRequest request(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
	session.sendRequest(request);
}


void STEAMGET::abort()
{
	session.abort();
}

void STEAMGET::stop()
{
	session.reset();
}


// --------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------


void STEAM::init(AbstractExt *extension)
{
	extension_ptr = extension;
	steam_get.init(extension);

	steam_run_flag = new std::atomic<bool>(false);

	STEAM_api_key = extension_ptr->pConf->getString("Steam.API Key", "");
	rconBanSettings.NumberOfVACBans = extension_ptr->pConf->getInt("STEAM.NumberOfVACBans", 1);
	rconBanSettings.DaysSinceLastBan = extension_ptr->pConf->getInt("STEAM.DaysSinceLastBan", 0);
	rconBanSettings.BanDuration = extension_ptr->pConf->getString("STEAM.BanDuration", "0");
	rconBanSettings.BanMessage = extension_ptr->pConf->getString("STEAM.BanMessage", "VAC Banned");

	SteamVacBans_Cache = new Poco::ExpireCache<std::string, SteamVACBans>(extension_ptr->pConf->getInt("STEAM.BanCacheTime", 3600000));
	SteamFriends_Cache = new Poco::ExpireCache<std::string, SteamFriends>(extension_ptr->pConf->getInt("STEAM.FriendsCacheTime", 3600000));
}


void STEAM::stop()
{
	*steam_run_flag = false;
}


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
		if (counter == 100)
		{
			steamIDs_str.erase(steamIDs_str.begin());
			steamIDs_str.pop_back();
			steamIDs_list.push_back(steamIDs_str);
		
			steamIDs_str.clear();
			counter = 0;
		}
		++counter;
		steamIDs_str += val + ",";
	}

	if (!steamIDs_str.empty())
	{
		steamIDs_str.pop_back();
		steamIDs_list.push_back(steamIDs_str);
	}
	return steamIDs_list;
}


void STEAM::updateSteamBans(std::vector<std::string> &steamIDs)
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

	Poco::Thread steam_thread;
	std::vector<std::string> steamIDStrings = generateSteamIDStrings(update_steamIDs);
	for (auto &steamIDString: steamIDStrings)
	{
		std::string query = "/ISteamUser/GetPlayerBans/v1/?key=" + STEAM_api_key + "&format=json&steamids=" + steamIDString;	

		steam_get.update(query);
		steam_thread.start(steam_get);
		try
		{
			steam_thread.join(360000);
			boost::property_tree::ptree pt;
			int response = steam_get.getResponse(pt);
			if (response == 1)
			{
				// SUCCESS STEAM QUERY
				for (const auto &val : pt.get_child("players"))
				{
					SteamVACBans steam_info;
					steam_info.steamID = val.second.get<std::string>("SteamId", "");
					steam_info.NumberOfVACBans = val.second.get<int>("NumberOfVACBans", 0);
					steam_info.VACBanned = val.second.get<bool>("VACBanned", false);
					steam_info.DaysSinceLastBan = val.second.get<int>("DaysSinceLastBan", 0);
					SteamVacBans_Cache->add(steam_info.steamID, steam_info);

					extension_ptr->console->info();
					extension_ptr->console->info("VAC Info: steamID {0}", steam_info.steamID);
					extension_ptr->console->info("VAC Info: NumberOfVACBans {0}", steam_info.NumberOfVACBans);
					extension_ptr->console->info("VAC Info: VACBanned {0}", steam_info.VACBanned);
					extension_ptr->console->info("VAC Info: DaysSinceLastBan {0}", steam_info.DaysSinceLastBan);

					if ((steam_info.NumberOfVACBans >= rconBanSettings.NumberOfVACBans) && (steam_info.DaysSinceLastBan <= rconBanSettings.DaysSinceLastBan))
					{
						steam_info.extDBBanned = true;
						extension_ptr->rconCommand("ban " + convertSteamIDtoBEGUID(steam_info.steamID) + " " + rconBanSettings.BanDuration + " " + rconBanSettings.BanMessage);
					}
				}
			}
			else if (response == 0)
			{
				// HTTP ERROR
			}
			else if (response == -1)
			{
				// ERROR STEAM QUERY
			}
		}
		catch (Poco::TimeoutException&)
		{
			steam_get.abort();
			steam_thread.join();
		}
	}
}


void STEAM::updateSteamFriends(std::vector<std::string> &steamIDs)
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

	Poco::Thread steam_thread;
	for (auto &steamID: update_steamIDs)
	{
		std::string query = "/ISteamUser/GetFriendList/v0001/?key=" + STEAM_api_key + "&relationship=friend&format=json&steamid=" + steamID;	

		steam_get.update(query);
		steam_thread.start(steam_get);
		try
		{
			steam_thread.join(360000); // 6 Minutes
			boost::property_tree::ptree pt;
			int response = steam_get.getResponse(pt);
			if (response == 1)
			{
				// SUCCESS STEAM QUERY
				SteamFriends steam_info;
				steam_info.friends.clear();
				for (const auto &val : pt.get_child("friendslist.friends"))
				{
					steam_info.steamID = val.second.get<std::string>("steamid", "");
					if (steam_info.steamID.empty())
					{
						steam_info.friends.push_back(steam_info.steamID);
					}
				}
				SteamFriends_Cache->add(steam_info.steamID, steam_info);
			}
			else if (response == 0)
			{
				// HTTP ERROR
			}
			else if (response == -1)
			{
				// ERROR STEAM QUERY
			}
		}
		catch (Poco::TimeoutException&)
		{
			steam_get.abort();
			steam_thread.join();
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
		extension_ptr->saveResult_mutexlock(unique_id, "[0,\"extDB: Steam Web API is not enabled\"]");
	}
}


void STEAM::run()
{
	*steam_run_flag = true;
	while (*steam_run_flag)
	{
		#ifdef TESTING
			extension_ptr->console->info("extDB: Steam: Sleep");
		#endif
		#ifdef DEBUG_LOGGING
			extension_ptr->logger->info("extDB: Steam: Sleep");
		#endif

		Poco::Thread::trySleep(60000); // 1 Minute Sleep unless woken up

		#ifdef TESTING
			extension_ptr->console->info("extDB: Steam: Wake Up");
		#endif
		#ifdef DEBUG_LOGGING
			extension_ptr->logger->info("extDB: Steam: Wake Up");
		#endif

		std::vector<SteamQuery> query_queue_copy;
		{
			boost::lock_guard<boost::mutex> lock(mutex_query_queue);
			query_queue_copy = query_queue;
			query_queue.clear();
		}

		if (!query_queue_copy.empty())
		{
			//bercon_thread.start(bercon);

			extension_ptr->console->info("Not Empty Queue");

			std::vector<std::string> steamIDs_friends;
			std::vector<std::string> steamIDs_bans;

			for (auto &val: query_queue_copy)
			{
				if (val.queryFriends)
				{
					extension_ptr->console->info("Query Friends: Size {0}", val.steamIDs.size());
					steamIDs_friends.insert(steamIDs_friends.end(), val.steamIDs.begin(), val.steamIDs.end());
				}
				if (val.queryVACBans)
				{
					extension_ptr->console->info("Query Bans: Size {0}", val.steamIDs.size());
					steamIDs_bans.insert(steamIDs_bans.end(), val.steamIDs.begin(), val.steamIDs.end());
				}
			}

			updateSteamFriends(steamIDs_friends);
			updateSteamBans(steamIDs_bans);

			std::string result;
			for (auto &val: query_queue_copy)
			{
				if (val.unique_id != -1)
				{
					result.clear();
					if (val.queryFriends)
					{
						for (auto &steamID: val.steamIDs)
						{
							Poco::SharedPtr<SteamFriends> info;
							info = SteamFriends_Cache->get(steamID);
							if (info.isNull())
							{
								result =+ "[],";
							}
							else
							{
								for (auto &friendSteamID: info->friends)
								{
									result = result +  ("\"" + friendSteamID + "\",");
								}
								if (result.empty())
								{
									result.pop_back();
								}
								result =+ "[" + result + "],";
							}
						}
					}
					else if (val.queryVACBans)
					{
						for (auto &steamID : val.steamIDs)
						{
							Poco::SharedPtr<SteamVACBans> info;
							info = SteamVacBans_Cache->get(steamID);
							if (info.isNull()) // Incase entry expired
							{
								result =+ "false,";
							}
							else
							{
								if (info->extDBBanned)
								{
									result =+ "true,";
								}
								else
								{
									result =+ "false,";
								}
							}
						}
					}
					if (!result.empty())
					{
						result.pop_back();
					}
					result = "[1,[" + result + "]]";
					extension_ptr->saveResult_mutexlock(val.unique_id, result);
				}
			}
		}
	}
}
