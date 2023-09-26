cmake_minimum_required(VERSION 3.12)
project(redis_from_scratch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(client src/client.cpp)


add_executable(server src/server.cpp)

