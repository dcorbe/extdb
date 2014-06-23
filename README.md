## extDB

WARNING

First time using c++, my main language i used before was python.

U been warned !!!!

---------------
---------------
---------------


### Arma3 Extension DB  C++ (windows / linux)

I got bored waiting on 2017 / epoch for Arma3...
So i decided to write up an C++ Extension for Arma3server.



#### Features:

 - Multi-Part Messages (i.e if output > outputsize set by arma)
 - Multi-Threading Sync / ASync Commands
 - Supports Mysql / SQlite / ODBC  (via Poco Library)

#### Requirements to Build

 - C++ Compiler
 - Poco Library http://pocoproject.org/  (note some more db features in 1.5 branch)
 - Boost Library http://www.boost.org/

 
#### Setup
Please note conf-main.ini + sqlite database if u are using one, 

both need to be in your arma3 directory and not @extdb.

Its my todo list to fix


#### Windows Binarys Provided are built using

Windows sdk 7.1 + VS2010sp1 + libboost 1.55.0 + poco-1.4.6p4 + tbb42_20140601oss
Using VS2010sp1 due to bug with boost + vs2013 (fix already in dev boost version).
Didn't feel it was worth time to figure out if bug would effect code


#### Linux Build Notes

Requirements
 - 32bit libraries of boost_filesystem boost_system boost_thread
 - 32bit Poco-1.4.6p4

Check your distro version of poco i.e poco-1.4.6p4 

Ubuntu is using debian stable version of poco (its abit old). 

Download latest version, compile + build + install into /usr/local etc ... (Readup on checkinstall, will make it easier to remove it in the future)

To compile type 
"make extdb"

cp extdb.so over to arma3 u can put into seperate directory if u like
i.e @extdb or just dump it into arma3 directory
