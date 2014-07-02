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



#include "db_raw.h"

#include <Poco/Data/MetaColumn.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>
#include <Poco/Exception.h>

#include <cstdlib>
#include <iostream>


std::string DB_RAW::callProtocol(AbstractExt *extension, std::string input_str)
{

    try
    {
		#ifdef TESTING
			std::cout << "extDB: DEBUG INFO: " + input_str << std::endl;
		#endif
		#ifdef LOGGING
			BOOST_LOG_SEV(logger, boost::log::trivial::trace) << " DB_RAW: " + input_str;
		#endif
		std::string result = "[";
		Poco::Data::Session db_session = extension->getDBSession_mutexlock();
		Poco::Data::Statement sql(db_session);
		sql << input_str;
		sql.execute();
		Poco::Data::RecordSet rs(sql);
		
		std::size_t cols = rs.columnCount();
		if (cols >= 1)
		{
			bool more = rs.moveFirst();
			while (more)
			{
				result += " [";
				for (std::size_t col = 0; col < cols; ++col)
				{
					if (rs.columnType(col) == Poco::Data::MetaColumn::FDT_STRING)
					{
						result += "\"" + (rs[col].convert<std::string>() + "\"");
					}
					else
					{
						result += rs[col].convert<std::string>();
					}
					if (col < (cols - 1))
					{
						result += ", ";
					}
				}

				more = rs.moveNext();
				if (more)
				{
					result += "],";
				}
				else
				{
					result += "]";
				}
			}
		}
		result += "]";
		return result;
	}
    catch (Poco::Exception& e)
    {
		#ifdef TESTING
			std::cout << "extDB: Error: " << e.displayText() << std::endl;
		#endif 
		#ifdef LOGGING
			BOOST_LOG_SEV(logger, boost::log::trivial::fatal) << " DB_RAW: Error: " + e.displayText();
		#endif
        return "[0,\"Error\"]";
    }
}