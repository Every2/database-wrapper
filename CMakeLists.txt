cmake_minimum_required(VERSION 3.12)
project(redis_from_scratch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(client src/client.cpp)
add_executable(servidoor src/server.cpp)

target_link_libraries(client PRIVATE ws2_32.lib)
target_link_libraries(servidoor PRIVATE ws2_32.lib)