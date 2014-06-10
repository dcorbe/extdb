# this is a example of Makefile
test:
	g++ -o extDB src/ext.cpp src/uniqueid.cpp src/plugins/abstractplugin.cpp -lPocoFoundation -lPocoData -lPocoDataODBC -lPocoDataSQLite -lPocoDataMySQL -lPocoUtil -lboost_filesystem -lboost_system -lboost_thread -fPIC -pipe -O2 -DTESTING -g

plugins:
	g++ -o db_raw.so src/plugins/db_raw.cpp src/plugins/abstractplugin.cpp -lPocoFoundation -lPocoData -lPocoDataODBC -lPocoDataSQLite -lPocoDataMySQL -lPocoUtil -lboost_filesystem -lboost_system -lboost_thread -shared -fPIC -pipe -O2 -g
	g++ -o misc.so src/plugins/misc.cpp src/plugins/abstractplugin.cpp -lPocoFoundation -lPocoData -lPocoDataODBC -lPocoDataSQLite -lPocoDataMySQL -lPocoUtil -lboost_filesystem -lboost_system -lboost_thread -shared -fPIC -pipe -O2 -g

test2: 
	g++ -o test ./test.cpp
