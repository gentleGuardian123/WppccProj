cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(WppccProj VERSION 1.0 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(SEAL_DIR ${CMAKE_SOURCE_DIR}/libs/lib/cmake/SEAL-4.1)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)
