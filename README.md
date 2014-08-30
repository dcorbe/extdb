## Arma3 Extension DB  C++ (windows / linux)   v16  

I got bored waiting on 2017 / Epoch for Arma3.
So i decided to write up an C++ Extension for Arma3server.


#### Missions / Mods using extDB
http://www.altisliferpg.com  


### Features

 - Multi-Part Messages (i.e if output > outputsize set by arma)
 - Multi-Threading Sync / ASync Commands
 - Unique ID for Messages
 - Supports Mysql / SQlite / ODBC
 - Support for Arma2 Randomizing config file


#### Protocols

 - DB_CUSTOM (Ability to define sql statements in a .ini file)
 - DB_BASIC
 - DB_PROCEDURE (limited support, no outputs)
 - DB_RAW (by raw i mean raw sql commands, no sanitizing input or output checks at all)
 - DB_RAW_NO_EXTRA_QUOTES
 - MISC (has beguid crc32 md4 md5 time + time offset)
 - MISC_LOG (ability to add info to extDB logfile)


#### Documentation @  
https://github.com/Torndeco/extDB/wiki

#### Install Windows
Look in windows_release/ pick a version i.e latest one.  

#### Crash C000005 ACCESS_VIOLATION
extDB doesn't work well with large datatypes in Database...  
   Check for Longtext (4GB) change it to MediumText / Text
 
#### Using Fred's Malloc
ExtDB is incompatible with https://github.com/fred41/tbbmalloc_arma.  
   Workaround rename Fred's malloc & change arma startup to the new filename.  

#### Requirements for Windows

 - Windows XP or greater
 - Windows Server 2003 or greater

#### Install from Source - Windows
https://github.com/Torndeco/extdb/wiki/Install

#### Install from Source - Linux
https://github.com/Torndeco/extdb/wiki/Install-Linux---Chroot-Guide-%28WIP%29


#### Thanks to

 - bladez- Using modified gplv3 code from https://github.com/bladez-/bercon for RCON
 - killzonekid for his blog + initial code in arma wiki for linux extensions
 - Everyone else i forgot, including EPOCH dev's for having an public github for mod at least on arma2 ;)
 - rajkosto for his work on DayZ Hive, that i have looked at & and still is over my head most of the time..   Using same method for getting Unique Char ID, also using almost the exact same boost parser for checking input/output to database   https://github.com/rajkosto/hive
 - Fank https://gist.github.com/Fank/11127158 for his code to convert SteamID to BEGUID.
 - Tonic & Atlis RPG Admins for beening literally beening bleeding edge testers. And good sports when i messed up, wasn't out a week and multiple servers were testing out extDB.
 - firefly2442 https://github.com/firefly2442 for the CMake Build System & wiki updates


##### Disclaimer
This is basically my first time using C++  
My main code experience is Python (pygtk + twisted), this is basically a hobby.
