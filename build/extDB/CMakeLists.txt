#Change this if you need to target a specific CMake version
cmake_minimum_required(VERSION 2.8)

cmake_policy(SET CMP0015 NEW)

set(CMAKE_USER_MAKE_RULES_OVERRIDE
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/cxx_flag_overrides.cmake)
project(extDB)

set(EXECUTABLE_NAME "extDB")

SET(SOURCES
	../../src/memory_allocator.cpp
	../../src/main.cpp
	../../src/ext.cpp
	../../src/uniqueid.cpp
	../../src/sanitize.cpp
	../../src/protocols/abstract_protocol.cpp
	../../src/protocols/db_procedure.cpp
	../../src/protocols/db_basic.cpp
	../../src/protocols/db_raw.cpp
	../../src/protocols/db_raw_no_extra_quotes.cpp
	../../src/protocols/misc_log.cpp
	../../src/protocols/misc.cpp
)

set(CMAKE_SUPPRESS_REGENERATION 1)
SET(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CONFIGURATION_TYPES Release CACHE TYPE  
INTERNAL FORCE )


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

# Look for BOOST
find_package(Boost REQUIRED COMPONENTS filesystem random regex system thread log)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
	LINK_DIRECTORIES(${Boost_LIBRARY_DIR})
else()
	message(FATAL_ERROR "\nBOOST not found\n Please Set BOOST_ROOT & BOOST_LIBRARYDIR")
endif()

# Look For POCO
include(FindPOCO)
if(POCO_FOUND)
	include_directories(${POCO_INCLUDE_DIR} ${POCO_Crypto_INCLUDE_DIR} ${POCO_Data_INCLUDE_DIR} ${POCO_Data_ODBC_INCLUDE_DIR})
	include_directories(${POCO_Data_SQLite_INCLUDE_DIR} ${POCO_Data_MYSQL_INCLUDE_DIR} ${POCO_Net_INCLUDE_DIR} ${POCO_Util_INCLUDE_DIR})
else()
	message(FATAL_ERROR "\nPOCO not found\n Please Set POCO_ROOT")
endif()


IF(DEFINED POCO_LIBRARYDIR)
	LINK_DIRECTORIES(${POCO_LIBRARYDIR})
else()	
	message(FATAL_ERROR "\nPOCO_LIBRARYDIR not found")
endif()

# Look for MySQL
include(FindMySQL)
if(MYSQL_FOUND)
	include_directories(${MYSQL_INCLUDE_DIR})
	LINK_DIRECTORIES(${MYSQL_LIB_DIR})
else()
	message(FATAL_ERROR "\nMYSQL not found")
endif()

# Look for ODBC
include(FindODBC)
if(ODBC_FOUND)
	include_directories(${ODBC_INCLUDE_DIR})
	#TODO: is the following needed?	
	LINK_DIRECTORIES(${ODBC_LIB_DIR})
else()
	message(FATAL_ERROR "\nODBC not found")
endif()


# Look for C++11 support BOOST_NO_CXX11_NULLPTR
include(FindCXX11)
if(CXX11_FOUND)
	message(STATUS "Adding C++11 support to compiler flags: " ${CXX11_FLAGS})
	#list(APPEND CMAKE_CXX_FLAGS "${CXX11_FLAGS}")
	SET (CMAKE_CXX_FLAGS "${CXX11_FLAGS}" CACHE STRING "" FORCE)
else()
	# TODO: Add check for NullPtr, i.e for VS2010
	message(STATUS "Compiler does not support C++11") 
endif()

# Look for Intel Threading Building Blocks (TBB)
include(FindTBB)
if(TBB_FOUND)
	include_directories(${TBB_INCLUDE_DIRS})
	LINK_DIRECTORIES(${TBB_LIBRARY_DIRS})
else()
	message(FATAL_ERROR "\nIntel TBB not found\n Please Set TBB_ROOT")
endif()


# Logging defaults to OFF
SET(DEBUG_LOGGING FALSE CACHE BOOL "Enables or disables Debug Logging.")
# Compile test application defaults to OFF
SET(COMPILE_TEST_APPLICATION FALSE CACHE BOOL "Compiles a test application of extdb.")
# Testing defaults to OFF
SET(TESTING FALSE CACHE BOOL "Sets the testing flag for extdb.")
# Test RCON defaults to OFF
SET(TEST_RCON FALSE CACHE BOOL "Enables or disables testing of RCON.")
# Test sanitize defaults to OFF
SET(TEST_SANITIZE FALSE CACHE BOOL "Enables or disables testing of sanitization.")


if (DEBUG_LOGGING)
	add_definitions(-DDEBUG_LOGGING)
	message(STATUS "Debug Logging is enabled.")
else()
	message(STATUS "Debug Logging is disabled.")
endif()

if (COMPILE_TEST_APPLICATION)
	add_definitions(-DTESTAPP)
	message(STATUS "The extdb test application will be compiled.")
	set(EXECUTABLE_NAME "extdb-test")
endif()

if (TEST_RCON)
	if (NOT TESTING)
		message(FATAL_ERROR "RCON testing was enabled but the TESTING flag is disabled.  Please enable it.")
	endif()
	add_definitions(-DTESTRCON)
	message(STATUS "RCON testing is enabled.")
endif()

if (TEST_SANITIZE)
	if (NOT TESTING)
		message(FATAL_ERROR "Sanitize testing was enabled but the TESTING flag is disabled.  Please enable it.")
	endif()
	add_definitions(-DTESTSANITIZE)
	message(STATUS "Sanitization testing is enabled.")
endif()

if (TESTING)
	add_definitions(-DTESTING)
	message(STATUS "Testing flag is enabled.")
	set(EXECUTABLE_NAME "extDB-test")
endif()


# Define sources and executable
add_library (${EXECUTABLE_NAME} SHARED ${SOURCES})
target_link_libraries(${EXECUTABLE_NAME} ${ODBC_LIBRARY})
target_link_libraries(${EXECUTABLE_NAME} ${MYSQL_LIBRARY})
target_link_libraries(${EXECUTABLE_NAME} ${TBB_LIBRARIES})

if(${WIN32})
	add_definitions(-D _WIN32_WINNT=0x0501)
	add_definitions(-D POCO_NO_UNWINDOWS)
	add_definitions(-DUNICODE -D_UNICODE)
	SET_TARGET_PROPERTIES(extDB PROPERTIES LINK_FLAGS " /MANIFEST:NO /ERRORREPORT:NONE")
else()
	add_definitions(-DBOOST_LOG_DYN_LINK)
endif()
