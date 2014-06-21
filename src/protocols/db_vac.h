/*
* Copyright (C) 2014 Declan Ireland <http://github.com/torndeco/extDB>
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/


#pragma once

#include <Poco/Data/SessionPool.h>

#include <cstdlib>
#include <iostream>

#include "../ext.h"

class DB_VAC: public AbstractProtocol
{
	public:
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

		bool querySteam(std::string steam_web_api_key, std::string steam_id, SteamVacInfo &vac_info);
		void updateVAC(Poco::Data::Session &db_session, std::string &steam_id);
};
