cmake_minimum_required(VERSION 3.10)

project(RobinhoodCpp)

#set(CMAKE_BUILD_TYPE Release)

#build with C++14
set(CMAKE_CXX_STANDARD 14)

find_package(CURL REQUIRED)
find_path(NLOHMANN_INCLUDE_DIR NAMES json.hpp PATH_SUFFIXES nlohmann)

#Bring the headers of the project
include_directories(./ authentication ${CURL_INCLUDE_DIRS} ${NLOHMANN_INCLUDE_DIR})
 
#manually add the sources using the set command 
set(SOURCES docs/example.cpp robinhoodtrader.cpp authentication/authentication.cpp authentication/base32.c authentication/hmac.c authentication/sha1.c authentication/util.c)
 
add_executable(RobinhoodCpp ${SOURCES})

target_link_libraries(RobinhoodCpp ${CURL_LIBRARIES})

