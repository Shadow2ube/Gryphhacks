cmake_minimum_required(VERSION 3.16)
project(Gryphhacks)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpqxx -lpq -pthread")

find_package(OpenSSL REQUIRED)
#find_package()

add_executable(Gryphhacks
        httplib.h json.hpp picosha2.h
        main.cpp
        util.h util.cpp)

target_link_libraries(Gryphhacks OpenSSL::SSL)