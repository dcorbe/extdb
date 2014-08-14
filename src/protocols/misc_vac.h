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

#ifdef TESTING
	#include <Poco/Data/SessionPool.h>
	#include <Poco/MD5Engine.h>
	#include <Poco/DigestEngine.h>

	#include <cstdlib>
	#include <iostream>

	#include "abstract_ext.h"
	#include "abstract_protocol.h"
	#include "../rcon.h"


	class DB_VAC: public AbstractProtocol
	{
		public:
			void init(AbstractExt *extension, const std::string init_str);
			std::string callProtocol(AbstractExt *extension, std::string input_str);
			
			
		private:
			struct SteamVacInfo {
				std::string SteamID;
				std::string VACBanned;
				std::string NumberOfVACBans;
				std::string DaysSinceLastBan;
				std::string EconomyBan;
				std::string LastChecked;
			};
			
			struct VacBanCheck {
				int NumberOfVACBans;
				int DaysSinceLastBan;
				std::string BanDuration;
				std::string BanMessage;
			};
			
			VacBanCheck vac_ban_check;

			bool isNumber(std::string &input_str);
			bool querySteam(std::string &steam_web_api_key, std::string &steam_id, SteamVacInfo &vac_info);
			void updateVAC(Rcon &rcon, std::string steam_web_api_key, Poco::Data::Session &db_session, std::string &steam_id);
			std::string convertSteamIDtoBEGUID(std::string &steamid);
			
			Poco::MD5Engine md5;
	};
#endif