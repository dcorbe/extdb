# this is a example of Makefile
main:
	g++ -shared -m32 -fPIC -pipe -O2 -o extdb.so src/main.cpp src/ext.cpp src/uniqueid.cpp src/protocols/abstract_protocol.cpp src/protocols/misc.cpp src/protocols/db_raw.cpp -lPocoFoundation -lPocoData -lPocoDataODBC -lPocoDataSQLite -lPocoDataMySQL -lPocoUtil -lboost_filesystem -lboost_system -lboost_thread

test:
	g++ -g -m32 -fPIC -pipe -O2 -o extdb src/ext.cpp src/uniqueid.cpp src/protocols/abstract_protocol.cpp src/protocols/misc.cpp src/protocols/db_raw.cpp -DTESTING -lPocoFoundation -lPocoData -lPocoDataODBC -lPocoDataSQLite -lPocoDataMySQL -lPocoUtil -lboost_filesystem -lboost_system -lboost_thread
