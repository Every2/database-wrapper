cmake_minimum_required(VERSION 3.12)
project(redis_from_scratch)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(SOURCES
    src/server.cc
    src/avl.cc
    src/hashtable.cc
    src/heap.cc
    src/thread_pool.cc
    src/zset.cc
)

add_executable(server ${SOURCES})

target_include_directories(server PUBLIC include)