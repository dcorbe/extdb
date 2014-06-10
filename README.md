## extDB

Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Still WIP (not READY) will crash + burn !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

Left Todo
Finish Raw DB Plugin
Code a Basic DB Plugin
Finish Misc Plugin

Double check multi-part messages are getting split up correctly


---------------
---------------
---------------


Arma3 Extension DB  C++ (windows / linux)

I got bored waiting on 2017 / epoch for Arma3...
So i decided to write up an C++ Extension for Arma3server.

extDB parse commands + passes onto plugins
extDB recieves string from plugins +  stores / sends results to server 
extDB uses Boost ASIO + threads for jobs 
extDB uses Poco Database for Database Connection
extDB uses Poco ClassLoader System to load other plugins i.e that do the actual work 

Example plugins provided
db_raw   -- for raw sql database commands
db_basic -- for a basic db queries etc (no raw access to db)
misc     -- generic commands i.e CRC32 / TIME

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


#### Linux Build Notes

Check your distro version of poco i.e poco-1.4.6p4

Ubuntu is using debian stable version of poco (its abit old). 
Download latest version, compile + build + install into /usr/local etc ... (Readup on checkinstall, will make it easier to remove it in the future)