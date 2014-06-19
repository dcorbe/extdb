# this is a example of Makefile
extdb:
	g++ -march=native -shared -m32 -fPIC -pipe -O2 -o extdb.so src/main.cpp src/ext.cpp src/uniqueid.cpp src/protocols/abstract_protocol.cpp src/protocols/misc.cpp src/protocols/db_raw.cpp -lPocoFoundation -lPocoData -lPocoDataODBC -lPocoDataSQLite -lPocoDataMySQL -lPocoUtil -lPocoXML -lboost_filesystem -lboost_system -lboost_thread -ltbb
	strip ./extdb.so

test:
	g++ -g -march=native -m32 -fPIC -pipe -O2 -o extdb-test src/ext.cpp src/uniqueid.cpp src/protocols/abstract_protocol.cpp src/protocols/misc.cpp src/protocols/db_raw.cpp -DTESTING -lPocoFoundation -lPocoData -lPocoDataSQLite -lPocoDataMySQL -lPocoDataODBC -lPocoUtil -lPocoXML -lboost_filesystem -lboost_system -lboost_thread
