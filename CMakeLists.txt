#Change this if you need to target a specific CMake version
cmake_minimum_required(VERSION 2.8)

project(extdb)

# Define sources and executable
set(EXECUTABLE_NAME "extdb")

SET(SOURCES
	src/main.cpp
	src/ext.cpp
	src/uniqueid.cpp
	src/sanitize.cpp
	src/protocols/abstract_protocol.cpp
	src/protocols/db_basic.cpp
	src/protocols/db_raw.cpp
	src/protocols/misc.cpp
)

add_executable(${EXECUTABLE_NAME} ${SOURCES})


set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake_modules" ${CMAKE_MODULE_PATH})


# Look for BOOST
find_package(Boost REQUIRED COMPONENTS filesystem system thread)
if(Boost_FOUND)
	include_directories(${Boost_INCLUDE_DIRS})
	target_link_libraries(${EXECUTABLE_NAME} ${Boost_LIBRARIES})   
else()
	message(FATAL_ERROR "BOOST not found")
endif()

# Look for Poco
find_library(POCO_FOUNDATION PocoFoundation)
find_library(POCO_CRYPTO PocoCrypto)
find_library(POCO_DATA PocoData)
find_library(POCO_DATA_ODBC NAMES PocoODBC PocoDataODBC)
find_library(POCO_DATA_SQLITE NAMES PocoSQLite PocoDataSQLite)
find_library(POCO_DATA_MYSQL NAMES PocoMySQL PocoDataMySQL)
find_library(POCO_NET PocoNet)
find_library(POCO_UTIL PocoUtil)
find_library(POCO_XML PocoXML)
target_link_libraries(${EXECUTABLE_NAME} ${POCO_FOUNDATION} ${POCO_CRYPTO} ${POCO_DATA} ${POCO_DATA_ODBC} 
${POCO_DATA_SQLITE} ${POCO_DATA_MYSQL} ${POCO_NET} ${POCO_UTIL} ${POCO_XML})


# Look for C++11 support
include(FindCXX11)
if(CXX11_FOUND)
	message(STATUS "Adding C++11 support to compiler flags: " ${CXX11_FLAGS})
	#list(APPEND CMAKE_CXX_FLAGS "${CXX11_FLAGS}")
	SET (CMAKE_CXX_FLAGS "${CXX11_FLAGS}" CACHE STRING "" FORCE)
else()
	message(FATAL_ERROR "Compiler does not support C++11")
endif()

# Install target
install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)

#TODO:
#Copy over license, documentation, etc. to install location
