## Arma3 Extension DB  C++ (windows / linux)

I got bored waiting on 2017 / epoch for Arma3...  
So i decided to write up an C++ Extension for Arma3server.

### Features

 - Multi-Part Messages (i.e if output > outputsize set by arma)
 - Multi-Threading Sync / ASync Commands
 - Supports Mysql / SQlite / ODBC  (via Poco Library)

#### Todo

 - Add Steam Friends Query to MISC
 - Sanitize DB Saving = Done, Need to Sanitize DB Reading
 - Update Documentation
 

#### Needs Testing (not enabled)

 - DB_VAC (Player VAC Ban Check)


#### Working

 - MISC (has crc32 md4 md5 time + time offset)
 - DB_RAW (by raw i mean raw sql commands, no santizing checks at all) this really should be used for testing purposes only.

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
 - rajkosto for his work on DayZ Hive, that i have looked at & and still is over my head most of the time..   Using same method for getting Unique Char ID, also using almost the exact same boost parser for checking input/output to database   https://github.com/rajkosto/hive

##### Disclaimer
This is basicly my first time using C++  
My main code experience is Python (pygtk + twisted), this is basicly a hobby.
