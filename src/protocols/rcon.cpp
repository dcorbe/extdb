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


#include "rcon.h"


bool RCON::init(AbstractExt *extension,  AbstractExt::DBConnectionInfo *database, const std::string init_str)
{
	extension_ptr = extension;
	//database_ptr = database;
	if (init_str.empty())
	{
		return extension_ptr->extDB_connectors_info.rcon;
	}
	else
	{
		#ifdef TESTING
			extension_ptr->console->warn("extDB: RCON: Init Options not implemented yet");
		#endif
		extension_ptr->logger->warn("extDB: RCON: Init Options not implemented yet");
		return false;
	}
}


bool RCON::callProtocol(std::string input_str, std::string &result, const int unique_id)
{
	/*
	Poco::StringTokenizer tokens(input_str, ":");
	if (tokens.count() < 2)
	{
		result = "[0,\"Error\"]";
	}
	else
	{
		if (boost::iequals(tokens[0], "loadBans") == 1)
		{

		}
		else if (boost::iequals(tokens[0], "ban") == 1)
		{
			
		}
		else if (boost::iequals(tokens[0], "addBan") == 1)
		{
			
		}
		else if (boost::iequals(tokens[0], "removeBan") == 1)
		{
			
		}
		else if (boost::iequals(tokens[0], "writeBans") == 1)
		{
			
		}
		else if (boost::iequals(tokens[0], "kick") == 1)
		{
			
		}
		else if (boost::iequals(tokens[0], "MaxPing") == 1)
		{
			
		}
	}

	tokens[0];
	extension_ptr->rconCommand(input_str);
	result = "[1]"; 
	*/
	return true;
}