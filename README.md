## Arma3 Extension DB  C++ (windows / linux)

I got bored waiting on 2017 / Epoch for Arma3.
So i decided to write up an C++ Extension for Arma3server.


#### Missions / Mods using extDB
http://www.altisliferpg.com  


### Features

 - Multi-Part Messages (i.e if output > outputsize set by arma)
 - Multi-Threading Sync / ASync Commands
 - Supports Mysql / SQlite / ODBC  (via Poco Library)


#### Working

 - MISC (has crc32 md4 md5 time + time offset)
 - DB_RAW (by raw i mean raw sql commands, no santizing input or output checks at all)
 - Support for Arma2 i.e randomize config file (DEV BRANCH ONLY ATM)


#### Working But Need to be Tested

 - DB_VAC (Player VAC Ban Check)  (DEV BRANCH ONLY ATM)
 - DB_PROCEDURE (DEV BRANCH ONLY ATM)
 - DB_BASIC


#### Install Windows
Look in windows_release/, pick a version i.e latest one.  
I recommend a logging version till u have a server up and running.  
Non-Logging is just a dll with no logging to help with performance.  
Edit the extdb-conf.ini. 

#### Extdb Known Issues with Windows
   ExtDB is incompatiable with https://github.com/fred41/tbbmalloc_arma.  
       As temp workaround rename Fred's malloc & change arma startup to the new filename.  
       I will look into renaming the extdb intel mallocs dlls to prevent this issue in the the next update.  
       
   If u still having issues try check u have http://www.microsoft.com/en-ie/download/details.aspx?id=5555 installed.  
       Haven't confirmed if i need it... but i be surprised if u dont have it installed.  

#### Requirements for Windows

 - Windows XP or greater
 - Windows 2003 or greater


#### Install from Source - Windows
Note new CMake Build System in the works atm.

https://github.com/Torndeco/extdb/wiki/Install

#### Install from Source - Linux
https://github.com/Torndeco/extdb/wiki/Install-Linux---Chroot-Guide-%28WIP%29

#### Extdb Known Issues with Linux

   None ? :) 
   Its annoying to have to setup a 32bit chroot enviroment.  
   But its alot easier than trying to install 32bit libraries on 64bit system without breaking something


#### Thanks to

 - bladez- Using modified gplv3 code from https://github.com/bladez-/bercon for RCON
 - killzonekid for his blog + initial code in arma wiki for linux extensions
 - Everyone else i forgot, including EPOCH dev's for having an public github for mod atleast on arma2 ;)
 - rajkosto for his work on DayZ Hive, that i have looked at & and still is over my head most of the time..   Using same method for getting Unique Char ID, also using almost the exact same boost parser for checking input/output to database   https://github.com/rajkosto/hive
 - Fank https://gist.github.com/Fank/11127158 for his code to convert SteamID to BEGUID.
 - Tonic & Atlis RPG Admins for beening literally beening bleeding edge testers. And good sports when i messed up, wasn't out a week and multiple servers were testing out extDB.
 - firefly2442 https://github.com/firefly2442 for work on new CMake Build System


##### Disclaimer
This is basicly my first time using C++  
My main code experience is Python (pygtk + twisted), this is basicly a hobby.
