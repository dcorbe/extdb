First static build for extDB :)

Poco::Log has bug aswell for static compiles and logging.
Switched over to Boost Logging, 
Note this means Debian Users can't compile from source without manually updating thier boost.

Requirements to build static build

Boost 1.56 (not a typo) there is a bug with older versions for static builds.
Use makefiles + not cmake for static builds...

make extdb-static