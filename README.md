extDB
=====

Coming Soon....
Left todo is basic plugin + some intial profile + benchmarking 

Arma3 Extension DB  C++ (windows / linux)

Features:
---------

 - Multi-Part Messages (i.e if output > outputsize set by arma)
 - Multi-Threading Sync / ASync Commands
 - Supports Mysql / SQlite / ODBC  (via Poco Library)
 - Plugin Support (Poco Class Loader) Dynamic Load Plugins @ startup of
   mission. *Only load plugins needed*


----------


Requirements to Build
---------------------

C++ Compiler
Poco Library http://pocoproject.org/
Boost Library http://www.boost.org/


Windows Binarys Provided are built using
----------------------------------------

Windows sdk 7.1 + VS2010sp1 + libboost 1.55.0 + poco-1.4.6p4


Linux Build Notes
-----------------

Check your distro version of poco i.e poco-1.4.6p4

Ubuntu is using debian stable version of poco (its abit old). 
Download latest version, compile + build + install into /usr/local etc ... (Readup on checkinstall, will make it easier to remove it in the future)

