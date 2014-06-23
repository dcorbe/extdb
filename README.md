## extDB (WIP)


### Arma3 Extension DB  C++ (windows / linux)

I got bored waiting on 2017 / epoch for Arma3...  
So i decided to write up an C++ Extension for Arma3server.

#### Features:

 - Multi-Part Messages (i.e if output > outputsize set by arma)
 - Multi-Threading Sync / ASync Commands
 - Supports Mysql / SQlite / ODBC  (via Poco Library)

#### Todo List:

 - Finish Off DB_VAC
 - Finish Off DB_BASIC

#### Working List


 - MISC (has crc32 md4 md5 time + time offset)
 - DB_RAW (by raw i mean raw sql commands, no santizing input) this really should be used for testing only.



#### Requirements to Build

 - C++ Compiler
 - Poco Library http://pocoproject.org/  (note some more db features in 1.5 branch)
 - Boost Library http://www.boost.org/


More info in Wiki (WIP)
https://github.com/Torndeco/extdb/wiki/Install
