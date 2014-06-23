## extDB (WIP)


### Arma3 Extension DB  C++ (windows / linux)

I got bored waiting on 2017 / epoch for Arma3...  
So i decided to write up an C++ Extension for Arma3server.

#### Features:

 - Multi-Part Messages (i.e if output > outputsize set by arma)
 - Multi-Threading Sync / ASync Commands
 - Supports Mysql / SQlite / ODBC  (via Poco Library)

#### Todo List:

 - Finish Off DB_VAC (steam vac check + ban player + with db support to prevent spaming steam servers with queries)
 - Finish Off DB_BASIC (basic database support with general functions)

#### Working List

 - MISC (has crc32 md4 md5 time + time offset)
 - DB_RAW (by raw i mean raw sql commands, no santizing input) this really should be used for testing only.


#### Requirements to Build

 - C++ Compiler
 - Poco Library http://pocoproject.org/  (note some more db features in 1.5 branch)
 - Boost Library http://www.boost.org/
 - Intel TBB https://www.threadingbuildingblocks.org/


####More info in Wiki (WIP)
https://github.com/Torndeco/extdb/wiki/Install


#### Thanks to

 - bladez- Using modified gplv3 code from https://github.com/bladez-/bercon for RCON
 - killzonekid for his blog + initial code in arma wiki for linux extensions
 - Everyone else i forgot, including EPOCH dev's for having an public github for mod atleast on arma2 ;)

##### Disclaimer
This is basicly my first time using C++, its been slow going but its getting there...
Before this my main language is twinkering with Arma SQF,  
Python + pygtk + twisted for a lobby gui client,  
and pyBEScanner a python log parser for battleelogs (kinda obsolete now, since battleye got regrex support)
