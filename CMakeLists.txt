#Change this if you need to target a specific CMake version
cmake_minimum_required(VERSION 2.8)

project(extDB)

# Define sources and executable
set(EXECUTABLE_NAME "extDB")

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

if(${WIN32})
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0501")

	add_definitions(-D WIN32_WINNT=0x0501)
	add_definitions(-D POCO_NO_UNWINDOWS)
endif()

# Look For POCO
include(FindPOCO)
include_directories(${POCO_INCLUDE_DIR} ${POCO_Crypto_INCLUDE_DIR} ${POCO_Data_INCLUDE_DIR} ${POCO_Data_ODBC_INCLUDE_DIR})
include_directories(${POCO_Data_SQLite_INCLUDE_DIR} ${POCO_Data_MYSQL_INCLUDE_DIR} ${POCO_Net_INCLUDE_DIR} ${POCO_Util_INCLUDE_DIR})



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
install(TARGETS ${EXECUTABLE_NAME} DESTINATION bin)

#TODO:
#Copy over license, documentation, etc. to install location
