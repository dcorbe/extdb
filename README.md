## extDB

Database Plugins still need to be done...
Due some v.basic benchmark to see if any major performance issue

Left Todo
Fix Raw DB Plugin
Code a Basic DB Plugin



---------------
---------------
---------------


### Arma3 Extension DB  C++ (windows / linux)

I got bored waiting on 2017 / epoch for Arma3...
So i decided to write up an C++ Extension for Arma3server.


#### Example plugins provided
- db_raw   -- for raw sql database commands
- db_basic -- for a basic db queries etc (no raw access to db)
- misc     -- generic commands i.e CRC32 / TIME

#### Features:

 - Multi-Part Messages (i.e if output > outputsize set by arma)
 - Multi-Threading Sync / ASync Commands
 - Supports Mysql / SQlite / ODBC  (via Poco Library)
 - Plugin Support (Poco Class Loader) Dynamic Load Plugins @ startup of
   mission. *Only load plugins needed*

#### Requirements to Build

 - C++ Compiler
 - Poco Library http://pocoproject.org/  (note some more db features in 1.5 branch)
 - Boost Library http://www.boost.org/


#### Windows Binarys Provided are built using

Windows sdk 7.1 + VS2010sp1 + libboost 1.55.0 + poco-1.4.6p4
Using VS2010sp1 due to bug with boost + vs2013 (fix already in dev boost version).
Didn't feel it was worth time to figure out if bug would effect code


#### Linux Build Notes

Check your distro version of poco i.e poco-1.4.6p4

Ubuntu is using debian stable version of poco (its abit old). 
Download latest version, compile + build + install into /usr/local etc ... (Readup on checkinstall, will make it easier to remove it in the future)
