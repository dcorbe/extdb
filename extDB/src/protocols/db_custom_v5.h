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
		void callProtocol(std::string input_str, std::string &result);
		
	private:
		Poco::MD5Engine md5;
		boost::mutex mutex_md5;

		std::string db_custom_name;
		Poco::AutoPtr<Poco::Util::IniFileConfiguration> template_ini;

		struct Value_Options {
			int number = -1;

			bool check;
			bool beguid = false;
			bool string = false;
			bool string_datatype_check = false;
			bool toArray_AltisLifeRpg = false;
			bool strip = false;
		};
		
		struct Template_Call {
			bool input_sanitize_value_check;
			bool output_sanitize_value_check;

			bool string_datatype_check;

			bool preparedStatement_cache;
			bool strip;
			std::string strip_chars;
			int strip_chars_action;

			int number_of_inputs;

			std::vector< std::string > sql_prepared_statements;
			std::vector< std::vector< Value_Options > > sql_inputs_options;
			std::vector< Value_Options > sql_outputs_options;
		};

		typedef std::unordered_map<std::string, Template_Call> Custom_Call_UnorderedMap;

		Custom_Call_UnorderedMap custom_calls;

		void callCustomProtocol(std::string call_name, std::unordered_map<std::string, Template_Call>::const_iterator custom_protocol_itr, std::vector< std::vector< std::string > > &all_processed_inputs, std::string &input_str, std::string &result);
		void executeSQL(Poco::Data::Statement &sql_statement, std::string &result, bool &status);

		void getBEGUID(std::string &input_str, std::string &result);
		void toArrayAltisLifeRpg(std::string &input_str, std::string &result, bool ToArray);
		void getResult(std::unordered_map<std::string, Template_Call>::const_iterator &custom_protocol_itr, Poco::Data::Statement &sql_statement, std::string &result);
};