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

#include <Poco/Data/Common.h>
#include <Poco/Data/MetaColumn.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/SessionPool.h>

#include <cstdlib>
#include <iostream>

#include "../ext.h"

class DB_BASIC: public AbstractProtocol
{
	public:
		std::string callProtocol(AbstractExt *extension, std::string input_str);
		
	private:
		void getCharUID(Poco::Data::Statement &sql, std::string &steamid, std::string &result);
		void getOptionAll(Poco::Data::Statement &sql, std::string &table, std::string &result);
		void getOption(Poco::Data::Statement &sql, std::string &table, std::string &uid, std::string &option, std::string &result);
		void setOption(Poco::Data::Statement &sql, std::string &table, std::string &uid, std::string &option, std::string &value, std::string &result);
};
