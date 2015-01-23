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


#pragma once

#include <boost/thread/thread.hpp>

#include <Poco/MD5Engine.h>
#include <Poco/DigestEngine.h>

#include <cstdlib>
#include <iostream>

#include "rcon.h"
#include "protocols/abstract_ext.h"


class STEAM: public Poco::Runnable
{
	public:
		void run();
		void stop();

		void init(AbstractExt *extension);
		void addQuery(const int &unique_id, bool queryFriends, bool queryVacBans, std::vector<std::string> &steamIDs);

	private:
		AbstractExt *extension_ptr;
		
		struct SteamVACBans
		{
			std::string steamID;
			bool VACBanned;
			int NumberOfVACBans;
			int DaysSinceLastBan;
			bool extDBBanned;
		};

		struct SteamFriends
		{
			std::string steamID;
			std::vector<std::string> friends;
		};
		
		struct SteamQuery
		{
			int unique_id;
			bool queryFriends;
			bool queryVACBans;
			std::vector<std::string> steamIDs;
		};

		struct RConBan
		{
			int NumberOfVACBans;
			int DaysSinceLastBan;
			std::string BanDuration;
			std::string BanMessage;
		};

		std::vector<SteamQuery> query_queue;
		boost::mutex mutex_query_queue;
		
		std::string STEAM_api_key;
		RConBan rconBanSettings;
		Poco::SharedPtr<Poco::ExpireCache<std::string, SteamVACBans> > SteamVacBans_Cache; // 1 Hour (3600000)
		Poco::SharedPtr<Poco::ExpireCache<std::string, SteamFriends> > SteamFriends_Cache; // 1 Hour (3600000)

		void updateSTEAMBans(std::vector<std::string> &steamIDs);
		std::string convertSteamIDtoBEGUID(const std::string &input_str);
		std::vector<std::string> generateSteamIDStrings(std::vector<std::string> &steamIDs);

		Poco::MD5Engine md5;
		boost::mutex mutex_md5;

		std::atomic<bool> steam_run_flag = false;
};