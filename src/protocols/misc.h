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

#include <boost/thread/thread.hpp>

#include <Poco/Checksum.h>
#include <Poco/ClassLibrary.h>
#include <Poco/Data/SessionPool.h>
#include <Poco/MD4Engine.h>
#include <Poco/MD5Engine.h>

#include <cstdlib>
#include <iostream>

#include "abstract_protocol.h"

class MISC: public AbstractPlugin
{
	public:

		std::string name() const
		{
			return "Plugin_MISC";
		}

		std::string callPlugin(AbstractExt *extension, std::string input_str);

		//Poco::Checksum checksum_adler32;
		//boost::mutex mutex_checksum_adler32;

		Poco::Checksum checksum_crc32;
		boost::mutex mutex_checksum_crc32;

private:
	
		Poco::MD5Engine md5;
		boost::mutex mutex_md5;
		Poco::MD4Engine md4;
		boost::mutex mutex_md4;

		std::string getDateTime();
		std::string getDateTime(int hours);
		//std::string getAdler32(std::string &str_input);
		std::string getCrc32(std::string &str_input);
		std::string getMD4(std::string &str_input);
		std::string getMD5(std::string &str_input);
};
