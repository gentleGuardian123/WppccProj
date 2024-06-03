cmake_minimum_required(VERSION 3.15)
project(MPSU)

include_directories(common)
include_directories(lowMC)
include_directories(circuit)
include_directories(shuffle)
include_directories(mpso)

# set(CMAKE_CXX_STANDARD 14)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)


file(GLOB_RECURSE SRCS
    common/*.cpp
    lowMC/*.cpp
    circuit/*.cpp
    shuffle/*.cpp
    mpso/*.cpp
)

add_executable(main ${SRCS})

find_package(volePSI REQUIRED HINTS "./libvolepsi")

set(CMAKE_BUILD_TYPE Release)
target_compile_options(main PUBLIC $<$<COMPILE_LANGUAGE:CXX>:-std=c++17> -lpthread -O3)
target_link_libraries(main visa::volePSI)

