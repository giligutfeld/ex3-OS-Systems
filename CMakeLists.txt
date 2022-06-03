cmake_minimum_required(VERSION 3.21)
project(ex3)

set(CMAKE_CXX_STANDARD 14)

SET(CMAKE_CXX_FLAGS -pthread)

add_executable(ex3 main.cpp)
