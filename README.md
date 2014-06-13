## extDB

This is now in a working state...

Work on windows dll, so build using static librarys
Windows works if u put dll & poco / boost dll into arma3 directory

Linux remember arma3 = 32bit, u need 32bit librarys of poco + boost (look @ makefile to see which boost u only need)


This has barely been tested...
Used mainly console app version to test the string outputsize

Haven't done any benchmarks yet in comparsion to inidb / arma2mysql yet



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


#### Windows Binarys Provided are built using

Windows sdk 7.1 + VS2010sp1 + libboost 1.55.0 + poco-1.4.6p4
Using VS2010sp1 due to bug with boost + vs2013 (fix already in dev boost version).
Didn't feel it was worth time to figure out if bug would effect code


#### Linux Build Notes

Check your distro version of poco i.e poco-1.4.6p4

Ubuntu is using debian stable version of poco (its abit old). 
Download latest version, compile + build + install into /usr/local etc ... (Readup on checkinstall, will make it easier to remove it in the future)


##### Removed Plugin Code...
Will revisit issue later to see if i can come with solution..
When i used libpoco classloader, it seems to cause arma3server to not be able to load dll for some reason :(