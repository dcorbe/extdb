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

#include <Poco/Data/Common.h>
#include <Poco/Data/MetaColumn.h>
#include <Poco/Data/RecordSet.h>
#include <Poco/Data/Session.h>
#include <Poco/Exception.h>

#include "Poco/Data/MySQL/Connector.h"
#include "Poco/Data/MySQL/MySQLException.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Data/SQLite/SQLiteException.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Data/SQLite/SQLiteException.h"
#include "Poco/Data/ODBC/Connector.h"
#include "Poco/Data/ODBC/ODBCException.h"

#include <cstdlib>
#include <iostream>



bool DB_RAW::init(AbstractExt *extension)
{
	if (extension->getDBType() == std::string("MySQL"))
	{
		return true;
	}
	else if (extension->getDBType() == std::string("ODBC"))
	{
		return true;
	}
	else if (extension->getDBType() == std::string("SQLite"))
	{
		return true;
	}
	else
	{
		// DATABASE NOT SETUP YET or SQLITE Doesn't Support Procedures
		#ifdef TESTING
			std::cout << "extDB: DB_RAW: No Database Connection" << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::trace) << "extDB: DB_RAW: No Database Connection";
		return false;
	}
}

void DB_RAW::callProtocol(AbstractExt *extension, std::string input_str, std::string &result)
{
    try
    {
		#ifdef TESTING
			std::cout << "extDB: DB_RAW: DEBUG INFO: " + input_str << std::endl;
		#endif
		#ifdef DEBUG_LOGGING
			BOOST_LOG_SEV(extension->logger, boost::log::trivial::trace) << "extDB: DB_RAW: " + input_str;
		#endif
		Poco::Data::Session db_session = extension->getDBSession_mutexlock();
		Poco::Data::Statement sql(db_session);
		sql << input_str;
		sql.execute();
		Poco::Data::RecordSet rs(sql);

		result = "[";
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
						if (!rs[col].isEmpty())
						{
							result += "\"" + (rs[col].convert<std::string>() + "\"");
						}
						else
						{
							result += ("\"\"");
						}
					}
					else
					{
						if (!rs[col].isEmpty())
						{
							result += rs[col].convert<std::string>();
						}
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
		#ifdef TESTING
			std::cout << "extDB: DEBUG INFO: RESULT:" + result << std::endl;
		#endif
		#ifdef DEBUG_LOGGING
			BOOST_LOG_SEV(extension->logger, boost::log::trivial::trace) << "extDB: DB_RAW: RESULT:" + result;
		#endif
	}
	catch (Poco::Data::MySQL::ConnectionException& e)
	{
		#ifdef TESTING
			std::cout << "extDB: Error: " << e.displayText() << std::endl;
		#endif 
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_RAW: Input: " + input_str;
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_RAW: Connection Exception: " << e.displayText();
		result = "[0,\"Error Connection Exception\"]";
	}
	catch(Poco::Data::MySQL::StatementException& e)
	{
		#ifdef TESTING
			std::cout << "extDB: Error: " << e.displayText() << std::endl;
		#endif 
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_RAW: Input: " + input_str;
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_RAW: Statement Exception: " << e.displayText();
		result = "[0,\"Error Statement Exception\"]";
	}
	catch (Poco::Data::DataException& e)
    {
		#ifdef TESTING
			std::cout << "extDB: Error: " << e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_RAW: Input: " + input_str;
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_RAW: Data Exception: " << e.displayText();
        result = "[0,\"Error Data Exception\"]";
    }
    catch (Poco::Exception& e)
	{
		#ifdef TESTING
			std::cout << "extDB: Error: " << e.displayText() << std::endl;
		#endif
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_RAW: Input: " + input_str;
		BOOST_LOG_SEV(extension->logger, boost::log::trivial::fatal) << "extDB: DB_RAW: Exception: " << e.displayText();
		result = "[0,\"Error Exception\"]";
	}
}