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

#include <boost/unordered_map.hpp>

#include <Poco/DynamicAny.h>
#include <Poco/StringTokenizer.h>

#include "abstract_ext.h"
#include "abstract_protocol.h"


class DB_CUSTOM_V2: public AbstractProtocol
{
	public:
		bool init(AbstractExt *extension, const std::string init_str);
		void callProtocol(AbstractExt *extension, std::string input_str, std::string &result);
		
	private:
		std::string db_custom_name;
		Poco::AutoPtr<Poco::Util::IniFileConfiguration> template_ini;
		
		struct Template_Calls {
			std::list<Poco::DynamicAny> sql;
			int number_of_inputs;
			bool sanitize_inputs;
			bool sanitize_outputs;
		};
		boost::unordered_map<std::string, Template_Calls> custom_protocol;

		void callCustomProtocol(AbstractExt *extension, boost::unordered_map<std::string, Template_Calls>::const_iterator itr, Poco::StringTokenizer &tokens, std::string &result);
};
