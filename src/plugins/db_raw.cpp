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


#include "db_raw.h"

#include "abstractplugin.h"

#include <Poco/ClassLibrary.h>
#include <Poco/Data/Common.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/RecordSet.h>

#include <cstdlib>
#include <iostream>


std::string DB_RAW::callPlugin(AbstractExt *extension, std::string input_str)
{

	//db_session << input_str, Poco::Data::into(result), Poco::Data::now;

	std::string result;
	Poco::Data::Session db_session = extension->getDBSession_mutexlock();
	Poco::Data::Statement select(db_session);
	select << input_str;
	std::cout << "1" << std::endl;
	select.execute();
	std::cout << "2" << std::endl;
	Poco::Data::RecordSet rs(select);
	if (rs.columnCount() > 1)
	{
		std::cout << "3" << std::endl;
		std::size_t cols = rs.columnCount();
		std::cout << "4" << std::endl;
		bool more = rs.moveFirst();
		std::cout << "5" << std::endl;
		while (more)
		{
			for (std::size_t col = 0; col < cols; ++col)
			{
				result += (rs[col].convert<std::string>() + ", ");
			}
			std::cout << std::endl;
			more = rs.moveNext();
		}
	}
	return ("\"" + result + "\"");
}

POCO_BEGIN_MANIFEST(AbstractPlugin)
	POCO_EXPORT_CLASS(DB_RAW)
POCO_END_MANIFEST

// optional set up and clean up functions
void pocoInitializeLibrary()
{
	//std::cout << "Plugin DB_RAW Library initializing" << std::endl;
}

void pocoUninitializeLibrary()
{
	//std::cout << "Plugin DB_RAW Library uninitializing" << std::endl;
}
