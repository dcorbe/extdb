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

#include <Poco/DynamicAny.h>
#include <Poco/StringTokenizer.h>
#include <Poco/MD5Engine.h>

#include <unordered_map>

#include "abstract_ext.h"
#include "abstract_protocol.h"


class DB_CUSTOM_V5: public AbstractProtocol
{
	public:
		bool init(AbstractExt *extension, const std::string init_str);
		void callProtocol(AbstractExt *extension, std::string input_str, std::string &result);
		
	private:
		Poco::MD5Engine md5;
		boost::mutex mutex_md5;

		std::string db_custom_name;
		Poco::AutoPtr<Poco::Util::IniFileConfiguration> template_ini;

		struct Value_Options {
			int input = -1;

			bool check;
			bool beguid = false;
			bool string = false;
			bool string_datatype_check = false;
		};
		
		struct Template_Call {
			int number_of_inputs;
			bool string_datatype_check;
			std::string bad_chars;
			std::string bad_chars_action; // TODO Change to INT

			bool input_sanitize_value_check;
			bool output_sanitize_value_check;

			std::vector< std::string > sql_prepared_statements;

			std::vector< std::vector< Value_Options > > sql_inputs_options;
			std::vector< Value_Options > sql_outputs_options;
		};

		std::unordered_map<std::string, Template_Call> custom_protocol;

		void callCustomProtocol(AbstractExt *extension, std::string call_name, std::unordered_map<std::string, Template_Call>::const_iterator itr, std::vector< std::vector< std::string > > &all_processed_inputs, std::string &input_str, std::string &result);
		void executeSQL(AbstractExt *extension, Poco::Data::Statement &sql_statement, std::string &result, bool &status);

		void getBEGUID(std::string &input_str, std::string &result);
		void getResult(std::unordered_map<std::string, Template_Call>::const_iterator itr, Poco::Data::Statement &sql_statement, std::string &result);
};