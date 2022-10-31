cmake_minimum_required(VERSION 3.0)
project(iqtools)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

add_subdirectory(src)
