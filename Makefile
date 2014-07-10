COMPILER := g++
CFLAGS := -march=native -m32 -fPIC -O2 -pipe -std=c++0x
LIBRARYS := -lPocoCrypto -lPocoFoundation -lPocoData -lPocoDataODBC -lPocoDataSQLite -lPocoDataMySQL -lPocoNet -lPocoUtil -lPocoXML -lboost_filesystem -lboost_random -lboost_regex -lboost_system -lboost_thread
FILES := src/ext.cpp src/uniqueid.cpp src/sanitize.cpp src/protocols/*.cpp


extdb:
	$(COMPILER) $(CFLAGS) -shared -o extDB.so $(FILES) src/main.cpp $(LIBRARYS) -DTESTING

extdb-logging:
	$(COMPILER) $(CFLAGS) -g -shared -o extDB.so $(FILES) src/main.cpp -DLOGGING -DBOOST_LOG_DYN_LINK $(LIBRARYS) -lboost_log -lboost_log_setup -lpthread

test:
	$(COMPILER) $(CFLAGS) -g -o extdb-test $(FILES) -DTESTAPP -DTESTING $(LIBRARYS)

test-logging:
	$(COMPILER) $(CFLAGS) -g -o extdb-test $(FILES) -DTESTAPP -DTESTING -DLOGGING -DBOOST_LOG_DYN_LINK $(LIBRARYS) -lboost_log -lboost_log_setup -lpthread

test-rcon:
	$(COMPILER) $(CFLAGS) -g -o extdb-test $(FILES) -DTESTRCON -DTESTING -DLOGGING -DBOOST_LOG_DYN_LINK $(LIBRARYS)

test-sanitize:
	$(COMPILER) $(CFLAGS) -g -o extdb-test src/sanitize.cpp -DTESTSANITIZE -DTESTING $(LIBRARYS) -lpthread -ldl
