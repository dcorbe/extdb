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
#include <boost/unordered_map.hpp>

#include <Poco/DynamicAny.h>
#include <Poco/StringTokenizer.h>

#include <Poco/MD5Engine.h>

#include "abstract_ext.h"
#include "abstract_protocol.h"


class DB_CUSTOM_V3: public AbstractProtocol
{
	public:
		bool init(AbstractExt *extension,  AbstractExt::DBConnectionInfo *database, const std::string init_str);
		void callProtocol(std::string input_str, std::string &result);
		
	private:
		Poco::MD5Engine md5;
		boost::mutex mutex_md5;

		std::string db_custom_name;
		Poco::AutoPtr<Poco::Util::IniFileConfiguration> template_ini;
		
		struct Template_Calls {
			int number_of_inputs;
			bool sanitize_value_check;
			bool string_datatype_check;
			std::string bad_chars;
			std::string bad_chars_action;
			std::vector< std::list<Poco::DynamicAny> > sql_statements;
		};
		boost::unordered_map<std::string, Template_Calls> custom_protocol;

		void callCustomProtocol(boost::unordered_map<std::string, Template_Calls>::const_iterator itr, std::vector< std::string > &tokens, bool &sanitize_value_check_ok, std::string &result);
		void getBEGUID(std::string &input_str, std::string &result);
};