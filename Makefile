
extDB:
	rm extDB.so; g++ -march=native -shared -m32 -fPIC -pipe -O2 -std=c++0x -o extDB.so src/main.cpp src/ext.cpp src/uniqueid.cpp src/rcon.cpp src/sanitize.cpp src/protocols/abstract_protocol.cpp src/protocols/misc.cpp src/protocols/db_raw.cpp -DBOOST_LOG_DYN_LINK -lPocoFoundation -lPocoData -lPocoDataODBC -lPocoDataSQLite -lPocoDataMySQL -lPocoNet -lPocoUtil -lPocoXML -lboost_filesystem -lboost_log -lboost_log_setup -lboost_system -lboost_thread -ltbb -lpthread
	strip ./extDB.so

extDB-logging:
	rm extDB.so; g++ -march=native -shared -m32 -fPIC -pipe -O2 -std=c++0x -o extDB.so src/main.cpp src/ext.cpp src/uniqueid.cpp src/rcon.cpp src/sanitize.cpp src/protocols/abstract_protocol.cpp src/protocols/misc.cpp src/protocols/db_raw.cpp -DLOGGING -DBOOST_LOG_DYN_LINK -lPocoFoundation -lPocoData -lPocoDataODBC -lPocoDataSQLite -lPocoDataMySQL -lPocoNet -lPocoUtil -lPocoXML -lboost_filesystem -lboost_log -lboost_log_setup -lboost_system -lboost_thread -ltbb -lpthread
	strip ./extDB.so

test:
	rm extDB-test; g++ -g -march=native -m32 -fPIC -pipe -O2 -std=c++0x -o extDB-test src/ext.cpp src/uniqueid.cpp src/rcon.cpp src/sanitize.cpp src/protocols/*.cpp -DTESTING -DLOGGING -DBOOST_LOG_DYN_LINK -lPocoFoundation -lPocoData -lPocoDataSQLite -lPocoDataMySQL -lPocoDataODBC -lPocoNet -lPocoUtil -lPocoXML -lboost_filesystem -lboost_log -lboost_log_setup -lboost_system -lboost_thread -lpthread

test-rcon:
	rm extDB-test-rcon; g++ -g -march=native -m32 -fPIC -pipe -o extDB-test-rcon src/rcon.cpp -DTESTING_RCON -lPocoFoundation -lPocoNet 

test-sanitize:
	rm extDB-test-sanitize; g++ -g -march=native -m32 -fPIC -pipe -O2 -std=c++0x -o extDB-test-sanitize src/sanitize.cpp -DTESTING3
