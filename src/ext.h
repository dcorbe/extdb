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

#include "plugins/abstractplugin.h"
#include "plugins/abstract_ext.h"

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/unordered_map.hpp>

#include <Poco/AutoPtr.h>
#include <Poco/ClassLoader.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/SessionPool.h>
#include <Poco/Manifest.h>
#include <Poco/Util/IniFileConfiguration.h>

typedef Poco::ClassLoader<AbstractPlugin> PluginLoader;

class Ext: public AbstractExt
{
	public:
		Ext();
		~Ext();

		void callExtenion(char *output, const int &output_size, const char *function);
		std::string versionMajorCheck() const;
		std::string versionMinorCheck() const;

		Poco::Data::Session getDBSession_mutexlock();
		void saveResult_mutexlock(const std::string &result, const int &unique_id);

	protected:
		Poco::AutoPtr<Poco::Util::IniFileConfiguration> pConf();

	private:
		bool extDB_lock;

		// ASIO Thread Queue
		boost::shared_ptr<boost::asio::io_service::work> io_work_ptr;
		boost::asio::io_service io_service;
		boost::mutex mutex_io_service;

		boost::thread_group threads;
		int max_threads;

		// Database Session Pool
		std::string db_type;
		boost::shared_ptr<Poco::Data::SessionPool> db_pool;
		boost::mutex mutex_db_pool;

		std::string connectDatabase(const std::string &conf_option);

		void getResult_mutexlock(const int &unique_id, char *output, const int &output_size);
		void sendResult_mutexlock(const std::string &result, char *output, const int &output_size);

		// boost::unordered_map + mutex -- for Plugin Name : Plugins Path
		boost::unordered_map< std::string, std::string > shared_map_plugins_path;
		boost::mutex mutex_shared_map_plugins_path;

		// boost::unordered_map + mutex -- for Plugin Loaded
		boost::unordered_map< std::string, boost::shared_ptr<AbstractPlugin> > shared_map_plugins;
		boost::mutex mutex_shared_map_plugins;

		// boost::unordered_map + mutex -- for Stored Results to long for outputsize
		boost::unordered_map<int, bool> shared_map_wait;
		boost::unordered_map<int, std::string> shared_map_results;
		boost::mutex mutex_shared_map;  // Using Same Lock for Wait / Results / Plugins

		// Unique ID for key for ^^
		boost::mutex mutex_unique_id;
		void freeUniqueID_mutexlock(const int &unique_id);
		int getUniqueID_mutexlock();

		// Plugins
		boost::shared_ptr< PluginLoader > loader;
		void addPlugin(const std::string &plugin, const std::string &protocol_name, char *output, const int &output_size);

		std::string syncCallPlugin(std::string protocol, std::string data);
		void onewayCallPlugin(const std::string protocol, const std::string data);
		void asyncCallPlugin(const std::string protocol, const std::string data, const int unique_id);
};
