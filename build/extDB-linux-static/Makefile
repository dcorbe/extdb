COMPILER := g++
CFLAGS := -march=native -fPIC -m32 -O2 -pipe -std=c++0x -static-libstdc++ -static-libgcc
STATIC_LIBRARYS := -lPocoCrypto -lPocoNet -lPocoUtil -lPocoXML -lmysqlclient -lPocoDataODBC -liodbc -lPocoDataMySQL -lPocoDataSQLite -lPocoData -lPocoFoundation -lmysqlclient_r -lboost_filesystem -lboost_chrono -lboost_date_time -lboost_program_options -lboost_random -lboost_regex -lboost_system -lboost_thread -lz
DYNAMIC_LIBRARYS := -ldl -lpthread
FILES := src/ext.cpp src/uniqueid.cpp src/sanitize.cpp src/protocols/*.cpp
extdb-static:
	$(COMPILER) $(CFLAGS) -g -shared -o extDB.so $(FILES) src/main.cpp -Wl,-Bstatic $(STATIC_LIBRARYS) -Wl,-Bdynamic $(DYNAMIC_LIBRARYS)
test:
	$(COMPILER) $(CFLAGS) -g -o extdb-test $(FILES) -DTEST_APP -DTESTING -Wl,-Bstatic $(STATIC_LIBRARYS) -Wl,-Bdynamic $(DYNAMIC_LIBRARYS)
