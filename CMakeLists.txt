cmake_minimum_required(VERSION 3.22)
project(Gryphhacks)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpqxx -lpq")

add_executable(Gryphhacks httplib.h json.hpp main.cpp)
