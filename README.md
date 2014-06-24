## Arma3 Extension DB  C++ (windows / linux)

I got bored waiting on 2017 / epoch for Arma3...  
So i decided to write up an C++ Extension for Arma3server.

### Features

 - Multi-Part Messages (i.e if output > outputsize set by arma)
 - Multi-Threading Sync / ASync Commands
 - Supports Mysql / SQlite / ODBC  (via Poco Library)

#### Todo

 - Finish Off DB_VAC (steam vac check + ban player + with db support to prevent spaming steam servers with queries)
 - Finish Off DB_BASIC (basic database support with general functions)
 - Add Steam Friends Query to MISC

#### Working

 - MISC (has crc32 md4 md5 time + time offset)
 - DB_RAW (by raw i mean raw sql commands, no santizing input) this really should be used for testing only.

#### Requirements

 - C++ Compiler
 - Poco Library http://pocoproject.org/  (note some more db features in 1.5 branch)
 - Boost Library http://www.boost.org/
 - Intel TBB https://www.threadingbuildingblocks.org/

#### Install
https://github.com/Torndeco/extdb/wiki/Install


#### Thanks to

 - bladez- Using modified gplv3 code from https://github.com/bladez-/bercon for RCON
 - killzonekid for his blog + initial code in arma wiki for linux extensions
 - Everyone else i forgot, including EPOCH dev's for having an public github for mod atleast on arma2 ;)

##### Disclaimer
This is basicly my first time using C++
Other code experience is Python (pygtk + twisted)
