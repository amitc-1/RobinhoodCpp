#CMake file for example using RobinhoodCpp

cmake_minimum_required(VERSION 3.10)
project(Example)

#set(CMAKE_BUILD_TYPE Release)

#build with C++14
set(CMAKE_CXX_STANDARD 14)

include_directories(../)
add_executable(example example.cpp)

find_library(ROBINHOODCPP_LIB NAMES RobinhoodCpp HINTS ${CMAKE_PREFIX_PATH}/build ${CMAKE_PREFIX_PATH}/build/Debug ${CMAKE_PREFIX_PATH}/build/Release)

target_link_libraries(example ${ROBINHOODCPP_LIB})

