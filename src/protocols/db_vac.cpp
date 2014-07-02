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



#include "db_vac.h"

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
#include <Poco/Path.h>
#include <Poco/URI.h>

#include <Poco/Exception.h>

#include <Poco/NumberParser.h>

#include <Poco/Types.h>
#include <Poco/MD5Engine.h>
#include <Poco/DigestEngine.h>

#include <string>

#include "../sanitize.h"


using namespace Poco::Data::Keywords;


void DB_VAC::init(AbstractExt *extension) {
	vac_ban_check.NumberOfVACBans = extension->pConf->getInt("VAC.NumberOfVACBans", 1);
	vac_ban_check.DaysSinceLastBan = extension->pConf->getInt("VAC.DaysSinceLastBan", 0);
	vac_ban_check.BanDuration = extension->pConf->getString("VAC.BanDuration", "0");
	vac_ban_check.BanMessage = extension->pConf->getString("VAC.BanMessage", "VAC BAN FOUND");
}


bool DB_VAC::isNumber(std::string &input_str)
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


std::string DB_VAC::convertSteamIDtoBEGUID(std::string &steamid)
// From Frank https://gist.github.com/Fank/11127158
// Modified to use libpoco
{
	Poco::Int64 steamID = Poco::NumberParser::parse64(steamid);
	//long long int steamID = Poco::NumberParser::parse64(steamid);
	Poco::Int8 i = 0, parts[8] = { 0 };
	//short int i = 0, parts[8] = { 0 };

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


bool DB_VAC::querySteam(std::string &steam_web_api_key, std::string &steam_id, SteamVacInfo &vac_info)
{
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
		return true;
	}
}

void DB_VAC::updateVAC(Rcon &rcon, std::string steam_web_api_key, Poco::Data::Session &db_session, std::string &steam_id)
{
	SteamVacInfo vac_info;
	bool status = querySteam(steam_web_api_key, steam_id, vac_info);
	if (status)
	{
		// Save to DB
		Poco::Data::Statement sql(db_session);
		sql << "INSERT INTO 'VAC BANS' (\"SteamID\", \"Number of Vac Bans\", \"Days Since Last Ban\", \"Last Check\") VALUES(:steamid, :number_of_bans, :days_since_last_bans, :last_check)", 
				use(vac_info.SteamID), use(vac_info.NumberOfVACBans), use(vac_info.DaysSinceLastBan), Poco::DateTime(), now;
		if ((Poco::NumberParser::parse(vac_info.NumberOfVACBans) >= vac_ban_check.NumberOfVACBans) && (Poco::NumberParser::parse(vac_info.DaysSinceLastBan) <=vac_ban_check.DaysSinceLastBan ))
		if (true)
		{
			rcon.sendCommand("ban " + convertSteamIDtoBEGUID(steam_id) + " " + vac_ban_check.BanDuration + " " + vac_ban_check.BanMessage);
		}
	}
	else
	{
		// FAILED STEAM QUERY
		#ifdef TESTING
		std::cout << "extdb: Steam VAC Query Failed: " + steam_id << std::endl;
		#endif
	}
}

std::string DB_VAC::callProtocol(AbstractExt *extension, std::string input_str)
{
	if (isNumber(input_str))
	{
		Poco::Data::Session db_session = extension->getDBSession_mutexlock();
		Poco::Data::Statement select(db_session);
		select << ("SELECT \"Number of Vac Bans\" FROM `VAC BANS` where SteamID="  + input_str), now;
		Poco::Data::RecordSet rs(select);
		
		if (rs.moveFirst())
		{
			Poco::DateTime last_check;
			Poco::DateTime now;
			int tzd;
			bool last_check_status = Poco::DateTimeParser::tryParse("%e-%n-%Y", rs.value("Last Check").convert<std::string>(), last_check, tzd);
			if (now - last_check >= Poco::Timespan(7*Poco::Timespan::DAYS))
			{
				updateVAC(extension->rcon, extension->getAPIKey(), db_session, input_str);
			}
		}
		else
		{
			updateVAC(extension->rcon, extension->getAPIKey(), db_session, input_str);
		}
		return ("[1]");
	}
	else
	{
		return ("[0,\"Error Invalid SteamID\"]");
	}
}