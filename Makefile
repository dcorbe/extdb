COMPILER := g++
CFLAGS := -march=native -m32 -fPIC -O2 -pipe -std=c++0x -DBOOST_LOG_DYN_LINK
LIBRARYS := -lPocoCrypto -lPocoFoundation -lPocoData -lPocoDataODBC -lPocoDataSQLite -lPocoDataMySQL -lPocoNet -lPocoUtil -lPocoXML -lboost_filesystem -lboost_log -lboost_log_setup -lboost_random -lboost_regex -lboost_system -lboost_thread -lpthread -ltbbmalloc
FILES := src/memory_allocator.cpp src/ext.cpp src/uniqueid.cpp src/sanitize.cpp src/protocols/*.cpp


extdb:
	$(COMPILER) $(CFLAGS) -shared -o extDB.so $(FILES) src/main.cpp $(LIBRARYS)

extdb-debug-logging:
	$(COMPILER) $(CFLAGS) -g -shared -o extDB.so $(FILES) src/main.cpp -DDEBUG_LOGGING $(LIBRARYS)

test:
	$(COMPILER) $(CFLAGS) -g -o extdb-test $(FILES) -DTESTAPP $(LIBRARYS)

test-debug-logging:
	$(COMPILER) $(CFLAGS) -g -o extdb-test $(FILES) -DTESTAPP -DTESTING -DDEBUG_LOGGING $(LIBRARYS)

test-rcon:
	$(COMPILER) $(CFLAGS) -g -o extdb-test $(FILES) -DTESTRCON -DTESTING -DDEBUG_LOGGING $(LIBRARYS)

test-sanitize:
	$(COMPILER) $(CFLAGS) -g -o extdb-test src/sanitize.cpp -DTESTSANITIZE -DTESTING $(LIBRARYS)
