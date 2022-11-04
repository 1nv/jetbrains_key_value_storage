cmake_minimum_required (VERSION 3.16)
project("JBKVS" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_library(jbkvs
 src/jbkvs/node.cpp
 src/jbkvs/storage.cpp
 src/jbkvs/storageNode.cpp
)
target_include_directories(jbkvs PUBLIC include)

enable_testing()
add_subdirectory(thirdparty/googletest)
add_executable(jbkvs_test
 tests/concurrentMap_test.cpp
 tests/node_test.cpp
 tests/storage_test.cpp
)
target_link_libraries(jbkvs_test PRIVATE GTest::gtest_main jbkvs)
add_test(jbkvs_tests jbkvs_test)
