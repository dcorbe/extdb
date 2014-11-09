COMPILER := g++
CFLAGS := -march=i686 -msse2 -msse3 -fPIC -m32 -O2 -pipe -std=c++0x
STATIC_LIBRARYS := -lPocoCrypto -lPocoUtil -lPocoDataMySQL -lPocoDataSQLite -lPocoData -lPocoFoundation -lmysqlclient -lboost_chrono -lboost_date_time -lboost_filesystem -lboost_log_setup -lboost_log -lboost_random -lboost_regex -lboost_system -lboost_thread -lz
DYNAMIC_LIBRARYS := -ldl -lpthread -ltbbmalloc
FILES := src/memory_allocator.cpp src/ext.cpp src/uniqueid.cpp src/sanitize.cpp src/protocols/abstract_protocol.cpp src/protocols/db_custom_v3.cpp src/protocols/db_procedure_v2.cpp src/protocols/db_raw_v2.cpp src/protocols/db_raw_no_extra_quotes_v2.cpp src/protocols/log.cpp src/protocols/misc.cpp

extdb-static:
	$(COMPILER) $(CFLAGS) -shared -o extDB.so $(FILES) src/main.cpp -Wl,-Bstatic $(STATIC_LIBRARYS) -Wl,-Bdynamic $(DYNAMIC_LIBRARYS)

extdb-static-debug-logging:
	$(COMPILER) $(CFLAGS) -shared -o extDB.so $(FILES) -DDEBUG_LOGGING src/main.cpp -Wl,-Bstatic $(STATIC_LIBRARYS) -Wl,-Bdynamic $(DYNAMIC_LIBRARYS)

test:
	$(COMPILER) $(CFLAGS) -g -o extdb-test $(FILES) -DTEST_APP -DTESTING -DDEBUG_LOGGING -Wl,-Bstatic $(STATIC_LIBRARYS) -Wl,-Bdynamic $(DYNAMIC_LIBRARYS)
