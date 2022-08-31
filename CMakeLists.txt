cmake_minimum_required(VERSION 3.23)
project(Gryphhacks)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -lpqxx -lpq -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I/usr/local/include/uWebSockets -L/usr/local/lib -lz -l:uSockets.a")

find_package(OpenSSL REQUIRED)
#find_package(websocketpp REQUIRED)
#find_package()

add_executable(Gryphhacks
        lib/json.hpp lib/picosha2.h
        main.cpp
        util.h util.cpp endpoints/misc.h endpoints/user.h settings.h endpoints/auth.h endpoints/event.h jwt.h hmac.h base64.h)

target_link_libraries(Gryphhacks OpenSSL::SSL OpenSSL::Crypto /usr/local/lib/uSockets.a)