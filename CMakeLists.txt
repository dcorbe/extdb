#Change this if you need to target a specific CMake version
cmake_minimum_required(VERSION 2.8)

cmake_policy(SET CMP0015 NEW)

set(CMAKE_USER_MAKE_RULES_OVERRIDE
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX
   ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/cxx_flag_overrides.cmake)
project(extDB)


SET(SOURCES
	src/main.cpp
	src/ext.cpp
	src/uniqueid.cpp
	src/sanitize.cpp
	src/protocols/abstract_protocol.cpp
	src/protocols/db_procedure.cpp
	src/protocols/db_basic.cpp
	src/protocols/db_raw.cpp
	src/protocols/misc.cpp
)

set(CMAKE_SUPPRESS_REGENERATION 1)
SET(CMAKE_BUILD_TYPE "Release")
set(CMAKE_CONFIGURATION_TYPES Release CACHE TYPE  
INTERNAL FORCE )


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})

# Look for BOOST
find_package(Boost REQUIRED COMPONENTS filesystem system thread)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
	LINK_DIRECTORIES(${Boost_LIBRARY_DIR})
else()
	message(FATAL_ERROR "\nBOOST not found\n Please Set BOOST_ROOT & BOOST_LIBRARYDIR")
endif()

#if(${WIN32})
	add_definitions(-D _WIN32_WINNT=0x0501)
	add_definitions(-D POCO_NO_UNWINDOWS)
#endif()

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
if(MYSQL_FOUND)
	include_directories(${ODBC_INCLUDE_DIR})
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



# Install target
#install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)

#TODO:
#Copy over license, documentation, etc. to install location

# Define sources and executable
add_library (extDB SHARED ${SOURCES})
##target_link_libraries(extDB ${Boost_LIBRARIES})
#target_link_libraries(${EXECUTABLE_NAME} ${Boost_LIBRARIES})
target_link_libraries(extDB ${ODBC_LIBRARY})
target_link_libraries(extDB ${MYSQL_LIBRARY})