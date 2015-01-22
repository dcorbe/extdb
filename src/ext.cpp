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

#include "ext.h"

#include <boost/algorithm/string.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/random/random_device.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/regex.hpp>

#include <Poco/Data/Session.h>
#include <Poco/Data/SessionPool.h>
#include <Poco/Data/MySQL/Connector.h>
#include <Poco/Data/MySQL/MySQLException.h>
#include <Poco/Data/SQLite/Connector.h>
#include <Poco/Data/SQLite/SQLiteException.h>

#include <Poco/AutoPtr.h>
#include <Poco/DateTime.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/Exception.h>
#include <Poco/NumberFormatter.h>
#include <Poco/NumberParser.h>
#include <Poco/StringTokenizer.h>
#include <Poco/Util/IniFileConfiguration.h>

#include <cstring>
#include <iostream>
#include <cstdlib>

#include "uniqueid.h"

#include "protocols/abstract_protocol.h"
#include "protocols/db_custom_v3.h"
#include "protocols/db_custom_v5.h"
#include "protocols/db_procedure_v2.h"
#include "protocols/db_raw_v3.h"
#include "protocols/log.h"
#include "protocols/misc.h"
#include "protocols/vac.h"



Ext::Ext(std::string dll_path) {
	try
	{
		mgr.reset (new IdManager);

		bool conf_found = false;
		bool conf_randomized = false;

		boost::filesystem::path extDB_config_path(dll_path);

		extDB_config_path = extDB_config_path.parent_path();
	  	extDB_config_path /= "extdb-conf.ini";

		std::string extDB_config_str = extDB_config_path.make_preferred().string();

		if (boost::filesystem::exists(extDB_config_str))
		{
			conf_found = true;
			extDB_info.path = extDB_config_path.parent_path().string();
		}
		else if (boost::filesystem::exists("extdb-conf.ini"))
		{
			conf_found = true;
			extDB_config_path = boost::filesystem::path("extdb-conf.ini");
			extDB_info.path = boost::filesystem::current_path().string();
		}
		else
		{
			#ifdef _WIN32	// WINDOWS ONLY, Linux Arma2 Doesn't have extension Support
				// Search for Randomize Config File -- Legacy Security Support For Arma2Servers
				extDB_config_path = extDB_config_path.parent_path();
				extDB_config_str = extDB_config_path.make_preferred().string();

				boost::regex expression("extdb-conf.*ini");

				if (!extDB_config_str.empty())
				{
					// CHECK DLL PATH FOR CONFIG
					for (boost::filesystem::directory_iterator it(extDB_config_str); it != boost::filesystem::directory_iterator(); ++it)
					{
						if (is_regular_file(it->path()))
						{
							if(boost::regex_search(it->path().string(), expression))
							{
								conf_found = true;
								conf_randomized = true;
								extDB_config_path = boost::filesystem::path(it->path().string());
								extDB_info.path = boost::filesystem::path (extDB_config_str).string();
								break;
							}
						}
					}
				}

				// CHECK ARMA ROOT DIRECTORY FOR CONFIG
				if (!conf_found)
				{
					for(boost::filesystem::directory_iterator it(boost::filesystem::current_path()); it !=  boost::filesystem::directory_iterator(); ++it)
					{
						if (is_regular_file(it->path()))
						{
							if(boost::regex_search(it->path().string(), expression))
							{
								conf_found = true;
								conf_randomized = true;
								extDB_config_path = boost::filesystem::path(it->path().string());
								extDB_info.path = boost::filesystem::current_path().string();
								break;
							}
						}
					}
				}
			#endif
		}

		// Initialize Logger
		Poco::DateTime now;
		std::string log_filename = Poco::DateTimeFormatter::format(now, "%H-%M-%S.log");

		boost::filesystem::path log_relative_path;
		log_relative_path = boost::filesystem::path(extDB_info.path);
		log_relative_path /= "extDB";
		log_relative_path /= "logs";
		log_relative_path /= Poco::DateTimeFormatter::format(now, "%Y");
		log_relative_path /= Poco::DateTimeFormatter::format(now, "%n");
		log_relative_path /= Poco::DateTimeFormatter::format(now, "%d");
		extDB_info.log_path = log_relative_path.make_preferred().string();
		boost::filesystem::create_directories(log_relative_path);
		log_relative_path /= log_filename;

		std::string extDB_belog_path;
		boost::filesystem::path belog_relative_path;
		belog_relative_path = boost::filesystem::path(extDB_info.path);
		belog_relative_path /= "extDB";
		belog_relative_path /= "logs";
		belog_relative_path /= Poco::DateTimeFormatter::format(now, "%Y");
		belog_relative_path /= Poco::DateTimeFormatter::format(now, "%n");
		belog_relative_path /= Poco::DateTimeFormatter::format(now, "%d");
		extDB_belog_path = belog_relative_path.make_preferred().string();
		boost::filesystem::create_directories(belog_relative_path);
		belog_relative_path /= log_filename;

		auto console_temp = spdlog::stdout_logger_mt("extDB Console logger");
		auto logger_temp = spdlog::daily_logger_mt("extDB File Logger", log_relative_path.make_preferred().string(), true);

		console.swap(console_temp);
		logger.swap(logger_temp);

		spdlog::set_level(spdlog::level::info);
		spdlog::set_pattern("[%H:%M:%S %z] [Thread %t] %v");


		logger->info("extDB: Version: {0}", getVersion());

		if (!conf_found)
		{
			console->critical("extDB: Unable to find extdb-conf.ini");
			logger->critical("extDB: Unable to find extdb-conf.ini");
			// Kill Server no config file found -- Evil
			std::exit(EXIT_SUCCESS);
		}
		else
		{
			pConf = (new Poco::Util::IniFileConfiguration(extDB_config_path.make_preferred().string()));
			#ifdef TESTING
				console->info("extDB: Found extdb-conf.ini");
			#endif
			logger->info("extDB: Found extdb-conf.ini");

			extDB_info.steam_web_api_key = pConf->getString("Main.Steam Web API Key", "");

			// Start Threads + ASIO
			extDB_info.max_threads = pConf->getInt("Main.Threads", 0);
			int detected_cpu_cores = boost::thread::hardware_concurrency();
			if (extDB_info.max_threads <= 0)
			{
				if (detected_cpu_cores > 6)
				{
					extDB_info.max_threads = 6;
					#ifdef TESTING
						console->info("extDB: Detected {0} Cores, Setting up {1} Worker Threads", detected_cpu_cores, extDB_info.max_threads);
					#endif
					logger->info("extDB: Detected {0} Cores, Setting up {1} Worker Threads", detected_cpu_cores, extDB_info.max_threads);
				}
			}
			else
			{
				if (detected_cpu_cores > 6)
				{
					extDB_info.max_threads = 6;
					#ifdef TESTING
						console->info("extDB: Detected {0} Cores, Setting up {1} Worker Threads", detected_cpu_cores, extDB_info.max_threads);
					#endif
					logger->info("extDB: Detected {0} Cores, Setting up {1} Worker Threads", detected_cpu_cores, extDB_info.max_threads);
				}
				else if (detected_cpu_cores < 0)
				{
					extDB_info.max_threads = 2;
					#ifdef TESTING
						console->info("extDB: Unable to Detect Cores, Setting up {1} Worker Threads", detected_cpu_cores, extDB_info.max_threads);
					#endif
					logger->info("extDB: Unable to Detect Cores, Setting up {1} Worker Threads", detected_cpu_cores, extDB_info.max_threads);
				}
				else
				{
					#ifdef TESTING
						console->info("extDB: Detected {0} Cores, Setting up {1} Worker Threads (config settings)", extDB_info.max_threads, 6);
					#endif
					logger->info("extDB: Detected {0} Cores, Setting up {1} Worker Threads (config settings)", extDB_info.max_threads, 6);
				}
			}
			
			io_work_ptr.reset(new boost::asio::io_service::work(io_service));
			for (int i = 0; i < extDB_info.max_threads; ++i)
			{
				threads.create_thread(boost::bind(&boost::asio::io_service::run, &io_service));
			}

			if (boost::iequals(pConf->getString("Log.Mode", "sync"), "async") == 1)
			{
				std::size_t q_size = 1048576; //queue size must be power of 2
				spdlog::set_async_mode(q_size);
			}

			if (pConf->getBool("Rcon.Enable", false))
			{
				auto belogger_temp = spdlog::daily_logger_mt("extDB BE File Logger", belog_relative_path.make_preferred().string(), true);
				belogger.swap(belogger_temp);
				extDB_connectors_info.rcon = true;
				serverRcon.reset(new Rcon(logger, std::string("127.0.0.1"), pConf->getInt("Rcon.Port", 2302), pConf->getString("Rcon.Password", "password")));
				serverRcon->run();
			}


			#ifdef _WIN32
				if ((pConf->getBool("Main.Randomize Config File", false)) && (!conf_randomized))
				// Only Gonna Randomize Once, Keeps things Simple
				{
					std::string chars("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
									  "1234567890");
					// Skipping Lowercase, this function only for arma2 + extensions only available on windows.
					boost::random::random_device rng;
					boost::random::uniform_int_distribution<> index_dist(0, chars.size() - 1);

					std::string randomized_filename = "extdb-conf-";
					for(int i = 0; i < 8; ++i) {
						randomized_filename += chars[index_dist(rng)];
					}
					randomized_filename += ".ini";

					boost::filesystem::path randomize_configfile_path = extDB_config_path.parent_path() /= randomized_filename;
					boost::filesystem::rename(extDB_config_path, randomize_configfile_path);
				}
			#endif
		}
	}
	catch (spdlog::spdlog_ex& e)
	{
		std::cout << "SPDLOG ERROR: " <<  e.what() << std::endl;
		std::exit(EXIT_FAILURE);
	}
}


Ext::~Ext(void)
{
	stop();
}


void Ext::stop()
{
	console->info("extDB: Stopping ...");
	logger->info("extDB: Stopping ...");
	io_service.stop();
	threads.join_all();
	if (extDB_connectors_info.mysql)
	{
		//Poco::Data::MySQL::Connector::unregisterConnector();
	}
	if (extDB_connectors_info.sqlite)
	{
		Poco::Data::SQLite::Connector::unregisterConnector();
	}
	if (extDB_connectors_info.rcon)
	{
		serverRcon->disconnect();
	}
	unordered_map_protocol.clear();
	unordered_map_wait.clear();
	unordered_map_results.clear();
}



std::string Ext::getVersion() const
{
	return "33";
}


std::string Ext::getExtensionPath()
{
	return extDB_info.path;
}


std::string Ext::getLogPath()
{
	return extDB_info.log_path;
}

std::string Ext::getAPIKey()
{
	return extDB_info.steam_web_api_key;
}


int Ext::getUniqueID_mutexlock()
{
	boost::lock_guard<boost::mutex> lock(mutex_unique_id);
	return mgr.get()->AllocateId();
}


void Ext::freeUniqueID_mutexlock(const int &unique_id)
{
	boost::lock_guard<boost::mutex> lock(mutex_unique_id);
	mgr.get()->FreeId(unique_id);
}


Poco::Data::Session Ext::getDBSession_mutexlock(AbstractExt::DBConnectionInfo &database)
// Gets available DB Session (mutex lock)
{
	boost::lock_guard<boost::mutex> lock(database.mutex_pool);
	return database.pool->get();
}


Poco::Data::Session Ext::getDBSession_mutexlock(AbstractExt::DBConnectionInfo &database, Poco::Data::SessionPool::SessionDataPtr &session_data_ptr)
// Gets available DB Session (mutex lock)
{
	boost::lock_guard<boost::mutex> lock(database.mutex_pool);
	return database.pool->get(session_data_ptr);
}


void Ext::connectDatabase(char *output, const int &output_size, const std::string &database_id, const std::string &database_conf)
{
	DBConnectionInfo *database;
	if (database_id.empty())
	{
		database = &extDB_connectors_info.database;
	}
	else
	{
		database = &extDB_connectors_info.database_extra[database_id];
	}

	bool failed = false;
	try
	{
		// Check if already connectted to Database.
		if (!database->type.empty())
		{
			#ifdef TESTING
				console->warn("extDB: Already Connected to Database");
			#endif
			logger->warn("extDB: Already Connected to a Database");
			std::strcpy(output, "[0,\"Already Connected to Database\"]");
		}
		else
		{
			if (pConf->hasOption(database_conf + ".Type"))
			{
				// Database
				database->type = pConf->getString(database_conf + ".Type");
				std::string db_name = pConf->getString(database_conf + ".Name");

				database->min_sessions = pConf->getInt(database_conf + ".minSessions", 1);
				if (database->min_sessions <= 0)
				{
					database->min_sessions = 1;
				}
				database->min_sessions = pConf->getInt(database_conf + ".maxSessions", 1);
				if (database->max_sessions <= 0)
				{
					database->max_sessions = extDB_info.max_threads;
				}

				database->idle_time = pConf->getInt(database_conf + ".idleTime", 600);

				#ifdef TESTING
					console->info("extDB: Database Type: {0}", database->type);
				#endif
				logger->info("extDB: Database Type: {0}", database->type);

				if (boost::iequals(database->type, std::string("MySQL")) == 1)
				{
					if (!(extDB_connectors_info.mysql))
					{
						Poco::Data::MySQL::Connector::registerConnector();
						extDB_connectors_info.mysql = true;
					}

					std::string username = pConf->getString(database_conf + ".Username");
					std::string password = pConf->getString(database_conf + ".Password");

					std::string ip = pConf->getString(database_conf + ".IP");
					std::string port = pConf->getString(database_conf + ".Port");

					database->connection_str = "host=" + ip + ";port=" + port + ";user=" + username + ";password=" + password + ";db=" + db_name + ";auto-reconnect=true";

					database->type = "MySQL";

					std::string compress = pConf->getString(database_conf + ".Compress", "false");
					if (boost::iequals(compress, "true") == 1)
					{
						database->connection_str += ";compress=true";
					}

					std::string auth = pConf->getString(database_conf + ".Secure Auth", "false");
					if (boost::iequals(auth, "true") == 1)
					{
						database->connection_str += ";secure-auth=true";	
					}

					database->pool.reset(new Poco::Data::SessionPool(database->type, 
																	 database->connection_str, 
																	 database->min_sessions, 
																	 database->max_sessions, 
																	 database->idle_time));
					if (database->pool->get().isConnected())
					{
						#ifdef TESTING
							console->info("extDB: Database Session Pool Started");
						#endif
						logger->info("extDB: Database Session Pool Started");
						std::strcpy(output, "[1]");
					}
					else
					{
						#ifdef TESTING
							console->warn("extDB: Database Session Pool Failed");
						#endif
						logger->warn("extDB: Database Session Pool Failed");
						std::strcpy(output, "[0,\"Database Session Pool Failed\"]");
						failed = true;
					}
				}
				else if (boost::iequals(database->type, "SQLite") == 1)
				{
					if (!(extDB_connectors_info.sqlite))
					{
						Poco::Data::SQLite::Connector::registerConnector();
						extDB_connectors_info.sqlite = true;
					}

					database->type = "SQLite";

					boost::filesystem::path sqlite_path(getExtensionPath());
					sqlite_path /= "extDB";
					sqlite_path /= "sqlite";
					sqlite_path /= "db_name";
					database->connection_str = sqlite_path.make_preferred().string();

					database->pool.reset(new Poco::Data::SessionPool(database->type, 
																	 database->connection_str, 
																	 database->min_sessions, 
																	 database->max_sessions, 
																	 database->idle_time));
					if (database->pool->get().isConnected())
					{
						#ifdef TESTING
							console->info("extDB: Database Session Pool Started");
						#endif
						logger->info("extDB: Database Session Pool Started");
						std::strcpy(output, "[1]");
					}
					else
					{
						#ifdef TESTING
							console->warn("extDB: Database Session Pool Failed");
						#endif
						logger->warn("extDB: Database Session Pool Failed");
						std::strcpy(output, "[0,\"Database Session Pool Failed\"]");
						failed = true;
					}
				}
				else
				{
					#ifdef TESTING
						console->warn("extDB: No Database Engine Found for {0}", db_name);
					#endif
					logger->warn("extDB: No Database Engine Found for {0}", db_name);
					std::strcpy(output, "[0,\"Unknown Database Type\"]");
					failed = true;
				}
			}
			else
			{
				#ifdef TESTING
					console->warn("extDB: No Config Option Found: {0}", database_conf);
				#endif
				logger->warn("extDB: No Config Option Found: {0}", database_conf);
				std::strcpy(output, "[0,\"No Config Option Found\"]");
				failed = true;
			}
		}
	}
	catch (Poco::Data::NotConnectedException& e)
	{
		#ifdef TESTING
			console->error("extDB: Database NotConnectedException Error: {0}", e.displayText());
		#endif
		logger->error("extDB: Database NotConnectedException Error: {0}", e.displayText());
		std::strcpy(output, "[0,\"Database NotConnectedException Error\"]");
		failed = true;
	}
	catch (Poco::Data::MySQL::ConnectionException& e)
	{
		#ifdef TESTING
			console->error("extDB: Database ConnectionException Error: {0}", e.displayText());
		#endif
		logger->error("extDB: Database ConnectionException Error: {0}", e.displayText());
		std::strcpy(output, "[0,\"Database ConnectionException Error\"]");
		failed = true;
	}
	catch (Poco::Exception& e)
	{
		#ifdef TESTING
			console->error("extDB: Database Exception Error: {0}", e.displayText());
		#endif
		logger->error("extDB: Database Exception Error: {0}", e.displayText());
		std::strcpy(output, "[0,\"Database Exception Error\"]");
		failed = true;
	}

	if (failed)
	{
		if (database_id.empty())
		{
			database->type.clear();
			database->connection_str.clear();
			database->pool.reset();
		}
		else
		{
			extDB_connectors_info.database_extra.erase(database_id);
		}
	}
}


void Ext::addProtocol(char *output, const int &output_size, const std::string &database_id, const std::string &protocol, const std::string &protocol_name, const std::string &init_data)
{
	DBConnectionInfo *database;
	if (database_id.empty())
	{
		database = &extDB_connectors_info.database;
	}
	else
	{
		database = &extDB_connectors_info.database_extra[database_id];
	}

	{
		boost::lock_guard<boost::mutex> lock(mutex_unordered_map_protocol);
		if (unordered_map_protocol.count(protocol_name) > 0)
		{
			std::strcpy(output, "[0,\"Error Protocol Name Already Taken\"]");
			logger->warn("extDB: Error Protocol Name Already Taken: {0}", protocol_name);
		}
		else
		{
			if (boost::iequals(protocol, std::string("MISC")) == 1)
			{
				unordered_map_protocol[protocol_name] = std::shared_ptr<AbstractProtocol> (new MISC());
				if (!unordered_map_protocol[protocol_name].get()->init(this, database, init_data))
				// Remove Class Instance if Failed to Load
				{
					unordered_map_protocol.erase(protocol_name);
					std::strcpy(output, "[0,\"Failed to Load Protocol\"]");
					logger->warn("extDB: Failed to Load Protocol");
				}
				else
				{
					std::strcpy(output, "[1]");
				}
			}
			else if (boost::iequals(protocol, std::string("LOG")) == 1)
			{
				unordered_map_protocol[protocol_name] = std::shared_ptr<AbstractProtocol> (new LOG());
				if (!unordered_map_protocol[protocol_name].get()->init(this, database, init_data))
				// Remove Class Instance if Failed to Load
				{
					unordered_map_protocol.erase(protocol_name);
					std::strcpy(output, "[0,\"Failed to Load Protocol\"]");
					logger->warn("extDB: Failed to Load Protocol");
				}
				else
				{
					std::strcpy(output, "[1]");
				}
			}
			else if (boost::iequals(protocol, std::string("VAC")) == 1)
			{
				unordered_map_protocol[protocol_name] = std::shared_ptr<AbstractProtocol> (new VAC());
				if (!unordered_map_protocol[protocol_name].get()->init(this, database, init_data))
				// Remove Class Instance if Failed to Load
				{
					unordered_map_protocol.erase(protocol_name);
					std::strcpy(output, "[0,\"Failed to Load Protocol\"]");
					logger->warn("extDB: Failed to Load Protocol");
				}
				else
				{
					std::strcpy(output, "[1]");
				}
			}
			else if (boost::iequals(protocol, std::string("DB_CUSTOM_V3")) == 1)
			{
				unordered_map_protocol[protocol_name] = std::shared_ptr<AbstractProtocol> (new DB_CUSTOM_V3());
				if (!unordered_map_protocol[protocol_name].get()->init(this, database, init_data))
				// Remove Class Instance if Failed to Load
				{
					unordered_map_protocol.erase(protocol_name);
					std::strcpy(output, "[0,\"Failed to Load Protocol\"]");
					logger->warn("extDB: Failed to Load Protocol");
				}
				else
				{
					std::strcpy(output, "[1]");
				}
			}
			else if (boost::iequals(protocol, std::string("DB_CUSTOM_V5")) == 1)
			{
				unordered_map_protocol[protocol_name] = std::shared_ptr<AbstractProtocol> (new DB_CUSTOM_V5());
				if (!unordered_map_protocol[protocol_name].get()->init(this, database, init_data))
				// Remove Class Instance if Failed to Load
				{
					unordered_map_protocol.erase(protocol_name);
					std::strcpy(output, "[0,\"Failed to Load Protocol\"]");
					logger->warn("extDB: Failed to Load Protocol");
				}
				else
				{
					std::strcpy(output, "[1]");
				}
			}
			else if (boost::iequals(protocol, std::string("DB_RAW_V3")) == 1)
			{
				unordered_map_protocol[protocol_name] = std::shared_ptr<AbstractProtocol> (new DB_RAW_V3());
				if (!unordered_map_protocol[protocol_name].get()->init(this, database, init_data))
				// Remove Class Instance if Failed to Load
				{
					unordered_map_protocol.erase(protocol_name);
					std::strcpy(output, "[0,\"Failed to Load Protocol\"]");
					logger->warn("extDB: Failed to Load Protocol");
				}
				else
				{
					std::strcpy(output, "[1]");
				}
			}
			else if (boost::iequals(protocol, std::string("DB_RAW_V2")) == 1)
			{
				unordered_map_protocol[protocol_name] = std::shared_ptr<AbstractProtocol> (new DB_RAW_V3());
				if (!unordered_map_protocol[protocol_name].get()->init(this, database, std::string("ADD_QUOTES")))
				// Remove Class Instance if Failed to Load
				{
					unordered_map_protocol.erase(protocol_name);
					std::strcpy(output, "[0,\"Failed to Load Protocol\"]");
					logger->warn("extDB: Failed to Load Protocol");
				}
				else
				{
					std::strcpy(output, "[1]");
				}
			}
			else if (boost::iequals(protocol, std::string("DB_RAW_NO_EXTRA_QUOTES_V2")) == 1)
			{
				unordered_map_protocol[protocol_name] = std::shared_ptr<AbstractProtocol> (new DB_RAW_V3());
				if (!unordered_map_protocol[protocol_name].get()->init(this, database, std::string()))
				// Remove Class Instance if Failed to Load
				{
					unordered_map_protocol.erase(protocol_name);
					std::strcpy(output, "[0,\"Failed to Load Protocol\"]");
					logger->warn("extDB: Failed to Load Protocol");
				}
				else
				{
					std::strcpy(output, "[1]");
				}
			}
			else if (boost::iequals(protocol, std::string("DB_PROCEDURE_V2")) == 1)
			{
				unordered_map_protocol[protocol_name] = std::shared_ptr<AbstractProtocol> (new DB_PROCEDURE_V2());
				if (!unordered_map_protocol[protocol_name].get()->init(this, database, init_data))
				// Remove Class Instance if Failed to Load
				{
					unordered_map_protocol.erase(protocol_name);
					std::strcpy(output, "[0,\"Failed to Load Protocol\"]");
					logger->warn("extDB: Failed to Load Protocol");
				}
				else
				{
					std::strcpy(output, "[1]");
				}
			}
			else
			{
				std::strcpy(output, "[0,\"Error Unknown Protocol\"]");
				logger->warn("extDB: Failed to Load Unknown Protocol: {0}", protocol);
			}
		}
	}
}


void Ext::getSinglePartResult_mutexlock(const int &unique_id, char *output, const int &output_size)
// Gets Result String from unordered map array -- Result Formt == Single-Message
//   If <=, then sends output to arma, and removes entry from unordered map array
//   If >, sends [5] to indicate MultiPartResult
{
	boost::lock_guard<boost::mutex> lock(mutex_unordered_map_results);
	std::unordered_map<int, std::string>::const_iterator it = unordered_map_results.find(unique_id);
	if (it == unordered_map_results.end()) // NO UNIQUE ID or WAIT
	{
		if (unordered_map_wait.count(unique_id) == 0)
		{
			std::strcpy(output, (""));
		}
		else
		{
			std::strcpy(output, ("[3]"));
		}
	}
	else // SEND MSG (Part)
	{
		if (it->second.length() > output_size)
		{
			std::strcpy(output, ("[5]"));
		}
		else
		{
			std::string msg = it->second.substr(0, output_size);
			std::strcpy(output, msg.c_str());
			unordered_map_results.erase(unique_id);
			freeUniqueID_mutexlock(unique_id);
		}
	}
}


void Ext::getMultiPartResult_mutexlock(const int &unique_id, char *output, const int &output_size)
// Gets Result String from unordered map array  -- Result Format = Multi-Message 
//   If length of String = 0, sends arma "", and removes entry from unordered map array
//   If <=, then sends output to arma
//   If >, then sends 1 part to arma + stores rest.
{
	boost::lock_guard<boost::mutex> lock(mutex_unordered_map_results);
	std::unordered_map<int, std::string>::const_iterator it = unordered_map_results.find(unique_id);
	if (it == unordered_map_results.end()) // NO UNIQUE ID or WAIT
	{
		if (unordered_map_wait.count(unique_id) == 0)
		{
			std::strcpy(output, (""));
		}
		else
		{
			std::strcpy(output, ("[3]"));
		}
	}
	else if (it->second.empty()) // END of MSG
	{
		unordered_map_results.erase(unique_id);
		freeUniqueID_mutexlock(unique_id);
		std::strcpy(output, (""));
	}
	else // SEND MSG (Part)
	{
		std::string msg = it->second.substr(0, output_size);
		std::strcpy(output, msg.c_str());
		if (it->second.length() > output_size)
		{
			unordered_map_results[unique_id] = it->second.substr(output_size);
		}
		else
		{
			unordered_map_results[unique_id].clear();
		}
	}
}


void Ext::saveResult_mutexlock(const std::string &result, const int &unique_id)
// Stores Result String  in a unordered map array.
//   Used when string > arma output char
{
	boost::lock_guard<boost::mutex> lock(mutex_unordered_map_results);
	unordered_map_results[unique_id] = "[1," + result + "]";
	unordered_map_wait.erase(unique_id);
}


void Ext::syncCallProtocol(char *output, const int &output_size, const std::string &protocol, const std::string &data)
// Sync callPlugin
{
	std::unordered_map< std::string, std::shared_ptr<AbstractProtocol> >::const_iterator itr = unordered_map_protocol.find(protocol);
	if (itr == unordered_map_protocol.end())
	{
		std::strcpy(output, ("[0,\"Error Unknown Protocol\"]"));
	}
	else
	{
		// Checks if Result String will fit into arma output char
		//   If <=, then sends output to arma
		//   if >, then sends ID Message arma + stores rest. (mutex locks)
		std::string result;
		result.reserve(2000);
		itr->second->callProtocol(data, result);
		if (result.length() <= (output_size-6))
		{
			std::strcpy(output, ("[1, " + result + "]").c_str());
		}
		else
		{
			const int unique_id = getUniqueID_mutexlock();
			saveResult_mutexlock(result, unique_id);
			std::strcpy(output, ("[2,\"" + Poco::NumberFormatter::format(unique_id) + "\"]").c_str());
		}
	}
}


void Ext::onewayCallProtocol(const std::string protocol, const std::string data)
// ASync callProtocol
{
	std::unordered_map< std::string, std::shared_ptr<AbstractProtocol> >::const_iterator itr = unordered_map_protocol.find(protocol);
	if (itr != unordered_map_protocol.end())
	{
		std::string result;
		result.reserve(2000);
		itr->second->callProtocol(data, result);
	}
}


void Ext::asyncCallProtocol(const std::string protocol, const std::string data, const int unique_id)
// ASync + Save callProtocol
// We check if Protocol exists here, since its a thread (less time spent blocking arma) and it shouldnt happen anyways
{
	std::string result;
	result.reserve(2000);
	unordered_map_protocol[protocol].get()->callProtocol(data, result);
	saveResult_mutexlock(result, unique_id);
}


void Ext::callExtenion(char *output, const int &output_size, const char *function)
{
	try
	{
		#ifdef DEBUG_LOGGING
			logger->info("extDB: Extension Input from Server: {0}", std::string(function));
		#endif

		const std::string input_str(function);

		std::string::size_type input_str_length = input_str.length();

		if (input_str_length <= 2)
		{
			std::strcpy(output, ("[0,\"Error Invalid Message\"]"));
			
				logger->info("extDB: Invalid Message: {0}", input_str);
		}
		else
		{
			const std::string sep_char(":");

			// Async / Sync
			int async;
			
			if (Poco::NumberParser::tryParse(input_str.substr(0,1), async))
			{

				switch (async)  // TODO Profile using Numberparser versus comparsion of char[0] + if statement checking length of *function
				{
					case 2: //ASYNC + SAVE
					{
						// Protocol
						const std::string::size_type found = input_str.find(sep_char,2);

						if (found==std::string::npos)  // Check Invalid Format
						{
							std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
							logger->error("extDB: Invalid Format: {0}", input_str);
						}
						else
						{
							const std::string protocol = input_str.substr(2,(found-2));

							if (found == (input_str_length - 1))
							{
								std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
								logger->error("extDB: Invalid Format: {0}", input_str);
							}
							else
							{
								bool found_procotol = false;
								// Data
								std::string data = input_str.substr(found+1);
								int unique_id = getUniqueID_mutexlock();

								// Check for Protocol Name Exists...
								// Do this so if someone manages to get server, the error message wont get stored in the result unordered map
								{
									if (unordered_map_protocol.find(protocol) != unordered_map_protocol.end())
									{
										std::strcpy(output, ("[0,\"Error Unknown Protocol\"]"));
										logger->error("extDB: Unknown Protocol: {0}", protocol);
									}
									else
									{
										boost::lock_guard<boost::mutex> lock(mutex_unordered_map_results);
										unordered_map_wait[unique_id] = true;
										found_procotol = true;
									}
								}
								// Only Add Job to Work Queue + Return ID if Protocol Name exists.
								if (found_procotol)
								{
									io_service.post(boost::bind(&Ext::asyncCallProtocol, this, protocol, data, unique_id));
									std::strcpy(output, (("[2,\"" + Poco::NumberFormatter::format(unique_id) + "\"]")).c_str());
								}
							}
						}
						break;
					}
					case 4: // GET -- Single-Part Message Format
					{
						const int unique_id = Poco::NumberParser::parse(input_str.substr(2));
						getSinglePartResult_mutexlock(unique_id, output, output_size);
						break;
					}
					case 5: // GET -- Multi-Part Message Format
					{
						const int unique_id = Poco::NumberParser::parse(input_str.substr(2));
						getMultiPartResult_mutexlock(unique_id, output, output_size);
						break;
					}
					case 1: //ASYNC
					{
						// Protocol
						const std::string::size_type found = input_str.find(sep_char,2);

						if (found==std::string::npos)  // Check Invalid Format
						{
							std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
							logger->error("extDB: Invalid Format: {0}", input_str);
						}
						else
						{
							if (found == (input_str_length - 1))
							{
								std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
								logger->error("extDB: Invalid Format: {0}", input_str);
							}
							else
							{
								const std::string protocol = input_str.substr(2,(found-2));
								// Data
								const std::string data = input_str.substr(found+1);
								io_service.post(boost::bind(&Ext::onewayCallProtocol, this, protocol, data));
								std::strcpy(output, "[1]");
							}
						}
						break;
					}
					case 0: //SYNC
					{
						// Protocol
						const std::string::size_type found = input_str.find(sep_char,2);

						if (found==std::string::npos)  // Check Invalid Format
						{
							std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
							logger->error("extDB: Invalid Format: {0}", input_str);
						}
						else
						{
							if (found == (input_str_length - 1))
							{
								std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
								logger->error("extDB: Invalid Format: {0}", input_str);
							}
							else
							{
								const std::string protocol = input_str.substr(2,(found-2));
								// Data
								const std::string data = input_str.substr(found+1);
								syncCallProtocol(output, output_size, protocol, data);
							};
						}
						break;
					}
					case 9:
					{
						Poco::StringTokenizer tokens(input_str, ":");
						if (extDB_info.extDB_lock)
						{
							if (tokens.count() == 2)
							{
								if (tokens[1] == "VERSION")
								{
									std::strcpy(output, getVersion().c_str());
								}
								else if (tokens[1] == "LOCK_STATUS")
								{
									std::strcpy(output, ("[1]"));
								}
								else
								{
									// Invalid Format
									std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
									logger->error("extDB: Invalid Format: {0}", input_str);
								}
							}
							else
							{
								// Invalid Format
								std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
								logger->error("extDB: Invalid Format: {0}", input_str);
							}
						}
						else
						{
							switch (tokens.count())
							{
								case 2:
									// LOCK / VERSION
									if (tokens[1] == "VERSION")
									{
										std::strcpy(output, getVersion().c_str());
									}
									else if (tokens[1] == "LOCK")
									{
										extDB_info.extDB_lock = true;
										std::strcpy(output, ("[1]"));
									}
									else if (tokens[1] == "LOCK_STATUS")
									{
										std::strcpy(output, ("[0]"));
									}
									else if (tokens[1] == "OUTPUTSIZE")
									{
										std::string outputsize_str(Poco::NumberFormatter::format(output_size));
										logger->info("Extension Output Size: {0}", outputsize_str);
										std::strcpy(output, outputsize_str.c_str());
									}
									else
									{
										std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
										logger->error("extDB: Invalid Format: {0}", input_str);
									}
									break;
								case 3:
									// DATABASE
									if (tokens[1] == "DATABASE")
									{
										connectDatabase(output, output_size, "", tokens[2]);
									}
									else
									{
										// Invalid Format
										std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
										logger->error("extDB: Invalid Format: {0}", input_str);
									}
									break;
								case 4:
									// ADD PROTOCOL
									if (tokens[1] == "ADD")
									{
										addProtocol(output, output_size, "", tokens[2], tokens[3], "");
									}
									else if (tokens[1] == "DATABASE_EXTRA")
									{
										connectDatabase(output, output_size, tokens[3], tokens[2]);
									}
									else
									{
										// Invalid Format
										std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
										logger->error("extDB: Invalid Format: {0}", input_str);
									}
									break;
								case 5:
									//ADD PROTOCOL
									if (tokens[1] == "ADD")
									{
										addProtocol(output, output_size, "", tokens[2], tokens[3], tokens[4]);
									}
									else if (tokens[1] == "ADD_EXTRA")
									{
										addProtocol(output, output_size, tokens[2], tokens[3], tokens[4], "");
									}
									else
									{
										// Invalid Format
										std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
										logger->error("extDB: Invalid Format: {0}", input_str);
									}
									break;
								case 6:
									if (tokens[1] == "ADD_EXTRA")
									{
										addProtocol(output, output_size, tokens[2], tokens[3], tokens[4], tokens[5]);
									}
									else
									{
										// Invalid Format
										std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
										logger->error("extDB: Invalid Format: {0}", input_str);
									}
									break;
								default:
									{
										// Invalid Format
										std::strcpy(output, ("[0,\"Error Invalid Format\"]"));
										logger->error("extDB: Invalid Format: {0}", input_str);
									}
							}
						}
						break;
					}
					default:
					{
						std::strcpy(output, ("[0,\"Error Invalid Message\"]"));
						logger->error("extDB: Invalid Message: {0}", input_str);
					}
				}
			}
			else
			{
				std::strcpy(output, ("[0,\"Error Invalid Message\"]"));
				#ifdef TESTING
					console->error("extDB: Invalid Message: {0}", input_str);
				#endif
				logger->error("extDB: Invalid Message: {0}", input_str);
			}
		}
	}
	catch (spdlog::spdlog_ex& e)
	{
		std::strcpy(output, ("[0,\"Error LOGGER\"]"));
		std::cout << "SPDLOG ERROR: " <<  e.what() << std::endl;
	}
	catch (Poco::Exception& e)
	{
		std::strcpy(output, ("[0,\"Error\"]"));
		#ifdef TESTING
			console->critical("extDB: Error: {0}", e.displayText());
		#endif
		logger->critical("extDB: Error: {0}", e.displayText());
	}
}


#ifdef TEST_APP
int main(int nNumberofArgs, char* pszArgs[])
{
	char result[80];
	std::string input_str;

	Ext *extension;
	std::string current_path;
	extension = (new Ext(current_path));

	extension->console->info("Welcome to extDB Test Application : ");
	extension->console->info("OutputSize is set to 80 for Test Application, just so it is readable ");
	extension->console->info("OutputSize for Arma3 is more like 10k in size ");
	extension->console->info("To exit type 'quit'");

	bool test = false;
	int test_counter = 0;
	for (;;) {
		std::getline(std::cin, input_str);
		if (input_str == "quit")
		{
		    break;
		}
		else if (input_str == "test")
		{
			test = true;
		}
		else
		{
			extension->callExtenion(result, 80, input_str.c_str());
			extension->console->info("extDB: {0}", result);
		}
		while (test)
		{
			if (test_counter >= 10000)
			{
				test_counter = 0;
				test = false;
				break;
			}
			test_counter = test_counter + 1;
			//extension->callExtenion(result, 80, std::string("1:SQL:SELECT * FROM PlayerData").c_str());
			extension->callExtenion(result, 80, std::string("1:SQL:TEST:testing").c_str());
			extension->console->info("extDB: {0}", result);			
		}
	}
	extension->stop();
	return 0;
}
#endif