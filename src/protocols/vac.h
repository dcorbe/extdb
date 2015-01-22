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
#include <Poco/Data/SessionPool.h>
#include <Poco/MD5Engine.h>
#include <Poco/DigestEngine.h>

#include <cstdlib>
#include <iostream>

#include "abstract_ext.h"
#include "abstract_protocol.h"
#include "../rcon.h"


class VAC: public AbstractProtocol
{
	public:
		bool init(AbstractExt *extension,  AbstractExt::DBConnectionInfo *database, const std::string init_str);
		void callProtocol(std::string input_str, std::string &result);
		
	private:
		struct SteamVacBans
		{
			std::string steamID;
			bool VACBanned;
			int NumberOfVACBans;
			int DaysSinceLastBan;
		};

		struct SteamVacFriends
		{
			std::string steamID;
			std::vector<std::string> friends;
		};
		
		struct VacBanCheck
		{
			int NumberOfVACBans;
			int DaysSinceLastBan;
			std::string BanDuration;
			std::string BanMessage;
		};
		
		VacBanCheck vac_ban_check;
		Poco::SharedPtr<Poco::ExpireCache<std::string, SteamVacBans> > VACBans_Cache; // 1 Hour (3600000)
		Poco::SharedPtr<Poco::ExpireCache<std::string, SteamVacFriends> > VACFriends_Cache; // 1 Hour (3600000)

		bool isNumber(const std::string &input_str);
		bool updateVAC(std::string steam_web_api_key, std::string &steam_id);
		bool convertSteamIDtoBEGUID(const std::string &input_str, std::string &result);

		Poco::MD5Engine md5;
		boost::mutex mutex_md5;
};