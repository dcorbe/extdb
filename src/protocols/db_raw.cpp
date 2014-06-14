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

#include <Poco/Data/Common.h>
#include <Poco/Data/MetaColumn.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>
#include <Poco/Exception.h>

#include <cstdlib>
#include <iostream>


std::string DB_RAW::callPlugin(AbstractExt *extension, std::string input_str)
{

    try
    {
		std::string result;
		Poco::Data::Session db_session = extension->getDBSession_mutexlock();
		Poco::Data::Statement select(db_session);
		select << input_str;
		select.execute();
		Poco::Data::RecordSet rs(select);
		if (rs.columnCount() >= 1)
		{
			std::size_t cols = rs.columnCount();
			bool more = rs.moveFirst();
			while (more)
			{
				result += " [";
				for (std::size_t col = 0; col < cols; ++col)
				{
					if (rs.columnType(col) == Poco::Data::MetaColumn::FDT_STRING)
					{
						result += "\"" + (rs[col].convert<std::string>() + "\"" + ", ");
					}
					else
					{
						result += (rs[col].convert<std::string>() + ", ");
					}
				}

				more = rs.moveNext();
				if (more)
				{
					result += "],";
				}
				else
				{
					result = result.substr(0, (result.length() - 2)) + "]";
				}
			}
		}
		return result;
	}
    catch (Poco::Exception& e)
    {
        return "[\"ERROR\",\"Error\"]";
        std::cout << "extDB: Error: " << e.displayText() << std::endl;
    }
}