cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-std=c++11)

project(DigiHarp)

find_package(raylib CONFIG REQUIRED)

add_executable(DigiHarp main.cpp)

target_link_libraries(DigiHarp PRIVATE raylib)