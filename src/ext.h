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

#include <boost/asio.hpp>
#include <boost/thread/thread.hpp>
#include <boost/unordered_map.hpp>

#include <Poco/AsyncChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>
#include <Poco/SimpleFileChannel.h>

#include <Poco/Thread.h>

#include "uniqueid.h"

#include "protocols/abstract_ext.h"
#include "protocols/abstract_protocol.h"


class DBPool : public Poco::Data::SessionPool
{
	public:
		DBPool(const std::string& sessionKey, const std::string& connectionString, int minSessions, int maxSessions, int idleTime): Poco::Data::SessionPool(sessionKey, connectionString, minSessions, maxSessions, idleTime)
		{
		}
		virtual ~DBPool()
		{
		}

		
	protected:
		void customizeSession (Poco::Data::Session& session);
};

class Ext: public AbstractExt
{
	public:
		Ext();
		~Ext();

		Poco::AutoPtr<Poco::SimpleFileChannel> pChannel;
		Poco::AutoPtr<Poco::AsyncChannel> pAsync;
		Poco::AutoPtr<Poco::PatternFormatter> pPF;
		Poco::AutoPtr<Poco::FormattingChannel> pPFC;

		void callExtenion(char *output, const int &output_size, const char *function);
		std::string version() const;

		Poco::AutoPtr<Poco::Util::IniFileConfiguration> pConf;

		Poco::Data::Session getDBSession_mutexlock();
		void saveResult_mutexlock(const std::string &result, const int &unique_id);
		void stop();

		std::string getAPIKey();
		std::string getDBType();

		int getUniqueID_mutexlock();
		void freeUniqueID_mutexlock(const int &unique_id);

	private:
		Poco::Logger *pLogger;
		bool extDB_lock;
		int max_threads;

		std::string steam_api_key;
		
		struct DBConnectionInfo {
			std::string db_type;
			std::string connection_str;
			int min_sessions;
			int max_sessions;
			int idle_time;
		};
		
		DBConnectionInfo db_conn_info;

		// ASIO Thread Queue
		boost::shared_ptr<boost::asio::io_service::work> io_work_ptr;
		boost::asio::io_service io_service;
		boost::mutex mutex_io_service;

		boost::thread_group threads;

		// Database Session Pool
		boost::shared_ptr<DBPool> db_pool;
		boost::mutex mutex_db_pool;

		void connectDatabase(char *output, const int &output_size, const std::string &conf_option);

		void getResult_mutexlock(const int &unique_id, char *output, const int &output_size);
		void sendResult_mutexlock(const std::string &result, char *output, const int &output_size);

		// boost::unordered_map + mutex -- for Plugin Loaded
		boost::unordered_map< std::string, boost::shared_ptr<AbstractProtocol> > unordered_map_protocol;
		boost::mutex mutex_unordered_map_protocol;

		// boost::unordered_map + mutex -- for Stored Results to long for outputsize
		boost::unordered_map<int, bool> unordered_map_wait;
		boost::unordered_map<int, std::string> unordered_map_results;
		boost::mutex mutex_unordered_map_results;  // Using Same Lock for Wait / Results / Plugins

		// Unique ID for key for ^^
		boost::shared_ptr<IdManager> mgr;
		boost::mutex mutex_unique_id;

		// Plugins
		void addProtocol(char *output, const int &output_size, const std::string &protocol, const std::string &protocol_name, const std::string &init_data);

		void syncCallProtocol(char *output, const int &output_size, const std::string &protocol, const std::string &data);
		void onewayCallProtocol(const std::string protocol, const std::string data);
		void asyncCallProtocol(const std::string protocol, const std::string data, const int unique_id);
};
