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

#include <Poco/Data/SessionPool.h>

#include <unordered_map>

#include "uniqueid.h"

#include "protocols/abstract_ext.h"
#include "protocols/abstract_protocol.h"



class Ext: public AbstractExt
{
	public:
		Ext(std::string path);
		~Ext();
		void stop();	
		void callExtenion(char *output, const int &output_size, const char *function);


	protected:
		std::string getAPIKey();
		std::string getExtensionPath();
		std::string getLogPath();

		void saveResult_mutexlock(const std::string &result, const int &unique_id);

		int getUniqueID_mutexlock();
		void freeUniqueID_mutexlock(const int &unique_id);

		Poco::Data::Session getDBSession_mutexlock(AbstractExt::DBConnectionInfo &database);
		Poco::Data::Session getDBSession_mutexlock(AbstractExt::DBConnectionInfo &database, Poco::Data::SessionPool::SessionDataPtr &session_data_ptr);


	private:
		struct extDBConnectors
		{
			bool rcon=false;
			bool mysql=false;
			bool sqlite=false;
			DBConnectionInfo database;
			std::unordered_map<std::string, DBConnectionInfo> database_extra;
		};

		struct extDBInfo
		{
			bool extDB_lock=false;
			int max_threads;

			std::string path;
			std::string log_path;
			std::string steam_web_api_key;
		};

		extDBConnectors extDB_connectors_info;
		extDBInfo extDB_info;

		// ASIO Thread Queue
		std::shared_ptr<boost::asio::io_service::work> io_work_ptr;
		boost::asio::io_service io_service;
		boost::mutex mutex_io_service;
		boost::thread_group threads;

		// RCon
		std::shared_ptr<Rcon> serverRcon;

		// std::unordered_map + mutex -- for Protocols Loaded
		std::unordered_map< std::string, std::shared_ptr<AbstractProtocol> > unordered_map_protocol;
		boost::mutex mutex_unordered_map_protocol;

		// std::unordered_map + mutex -- for Stored Results to long for Inputsize
		std::unordered_map<int, bool> unordered_map_wait;
		std::unordered_map<int, std::string> unordered_map_results;
		boost::mutex mutex_unordered_map_results;  // Using Same Lock for Wait / Results / Plugins

		// Unique ID for key for ^^
		std::shared_ptr<IdManager> mgr;
		boost::mutex mutex_unique_id;

		void connectDatabase(char *output, const int &output_size, const std::string &database_id, const std::string &database_conf);

		void getSinglePartResult_mutexlock(const int &unique_id, char *output, const int &output_size);
		void getMultiPartResult_mutexlock(const int &unique_id, char *output, const int &output_size);
		void sendResult_mutexlock(const std::string &result, char *output, const int &output_size);

		// Protocols
		void addProtocol(char *output, const int &output_size, const std::string &database_id, const std::string &protocol, const std::string &protocol_name, const std::string &init_data);
		void syncCallProtocol(char *output, const int &output_size, const std::string &protocol, const std::string &data);
		void onewayCallProtocol(const std::string protocol, const std::string data);
		void asyncCallProtocol(const std::string protocol, const std::string data, const int unique_id);

		// Version Info
		std::string getVersion() const;
};