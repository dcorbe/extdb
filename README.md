## Arma3 Extension DB  C++ (windows / linux)   v32 

I got bored waiting on 2017 / Epoch for Arma3.  
So i decided to write up an C++ Extension for Arma3server.


#### Known Missions / Mods using extDB
http://www.altisliferpg.com  
http://a3wasteland.com/


### Features

 - Rcon Support
 - Steam VAC + Friends Queries
 - Multi-Part Messages (i.e if output > outputsize set by arma)
 - Multi-Threading Sync / ASync Commands
 - Unique ID for Messages
 - Supports Mysql / SQlite / ODBC
 - Supports Prepared Statements via DB_CUSTOM_V5
 - Support for Arma2 Randomizing config file


#### Protocols

 - DB_CUSTOM_V3 (Ability to define sql statements in a .ini file)
 - DB_CUSTOM_V5 (Ability to define sql prepared statements in a .ini file)
 - DB_PROCEDURE_V2 (limited support, no outputs)
 - DB_RAW_V3 (by raw i mean raw sql commands, no sql escaping etc)
 - MISC (has beguid crc32 md4 md5 time + time offset)
 - LOG (Ability to log info into custom log files)
 - VAC (Ability to Query Steam for Ban / Friend Info)


#### Documentation @  
https://github.com/Torndeco/extDB/wiki

#### Linux Requirements  
Linux Distro with Glibc 2.17 or higher  
Debian 8 / Centos 7 / Ubuntu 14.10  

#### Windows Requirements  
Windows Server 2008 + Later  
Windows 7 + Later  

Install vcredist_x86.exe @ http://www.microsoft.com/en-ie/download/details.aspx?id=40784  

 
#### Using Fred's Malloc
extDB is incompatible with https://github.com/fred41/tbbmalloc_arma.  
   Workaround rename Fred's malloc & change arma startup to the new filename.  

#### Thanks to

 - bladez- Using modified code from https://github.com/bladez-/bercon for RCON  
 - Fank for his code to convert SteamID to BEGUID https://gist.github.com/Fank/11127158

 - rajkosto for his work on DayZ Hive, using almost the exact same boost parser for sanitize checks for input/output https://github.com/rajkosto/hive  
 - firefly2442 for the CMake Build System & wiki updates https://github.com/firefly2442
 - MaHuJa for fixing Test Application Input, no longer hardcoded input limit https://github.com/MaHuJa

 - killzonekid for his blog http://killzonekid.com/
  
 - Tonic & Atlis RPG Admins for beening literally beening bleeding edge testers for extDB. https://github.com/TAWTonic

 - Gabime for Spdlog https://github.com/gabime/spdlog



##### Disclaimer
This is basically my first time using C++  
My main code experience is Python (pygtk + twisted), this is basically a hobby.
