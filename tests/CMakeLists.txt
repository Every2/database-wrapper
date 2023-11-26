enable_testing()

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(test_database test.cpp)

target_link_libraries(test_database gtest gtest_main)

add_test(test_database test_database)