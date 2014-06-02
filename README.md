extDB
=====

Coming Soon....
Just need to code some plugins + profile 



Arma3 Extension DB  C++ dll / so   (windows / linux)

Features:
Multi-Part Messages (i.e if output > outputsize set by arma)
Multi-Threading Sync / ASync Commands

Supports Mysql / SQlite / ODBC  (via Poco Library)

Plugin Support (Poco Class Loader)
Dynamic Load Plugins @ startup of mission. Only load plugins needed (if dont want raw_db plugin loaded)


Requirements to Build
C++ Compiler
Poco Library http://pocoproject.org/
Boost Library http://www.boost.org/

Windows Binarys Provided are built using
VS2010sp1 + libboost 1.55.0 + poco(version still to decided) + windows sdk 7.1
