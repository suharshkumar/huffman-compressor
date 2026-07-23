cmake_minimum_required(VERSION 3.14)
project(huffman_compressor CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

# --- Core library -----------------------------------------------------------
add_library(huffman src/Huffman.cpp)
target_include_directories(huffman PUBLIC include)

# --- CLI executable ---------------------------------------------------------
add_executable(huffman_cli src/main.cpp)
set_target_properties(huffman_cli PROPERTIES OUTPUT_NAME huffman)
target_link_libraries(huffman_cli PRIVATE huffman)

# --- Tests (GoogleTest, fetched automatically) ------------------------------
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_executable(huffman_tests tests/test_huffman.cpp)
target_link_libraries(huffman_tests PRIVATE huffman GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(huffman_tests)
