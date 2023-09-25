cmake_minimum_required(VERSION 3.12)
project(redis_from_scratch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(test_client changes/client.cpp)


add_executable(test_server changes/server.cpp)
