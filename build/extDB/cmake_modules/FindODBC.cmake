IF (ODBC_INCLUDE_DIR)
  # Already in cache, be silent
  SET(ODBC_FIND_QUIETLY TRUE)
ENDIF (ODBC_INCLUDE_DIR)


FIND_PATH(ODBC_INCLUDE_DIR odbcinst.h
  /usr/local/include/mysql
  /usr/include/mysql
  "C:/Program Files/ODBC/include"
  "C:/ODBC/include"
  "C:/Program Files (x86)/Microsoft SDKs/Windows/v7.0A/include"
)

FIND_PATH(ODBC_LIB_DIR odbc32.lib
  /usr/local/include/mysql
  /usr/include/mysql
  "C:/Program Files/ODBC/lib"
  "C:/ODBC/lib"
  "C:/Program Files (x86)/Microsoft SDKs/Windows/v7.0A/Lib"
)

FIND_LIBRARY(ODBC_LIBRARY
  NAMES iodbc odbc odbcinst odbc32
  PATHS
  /usr/lib
  /usr/lib/odbc
  /usr/local/lib
  /usr/local/lib/odbc
  /usr/local/odbc/lib
  "C:/Program Files/ODBC/lib"
  "C:/ODBC/lib/debug"
  "C:/Program Files (x86)/Microsoft SDKs/Windows/v7.0A/Lib"
)