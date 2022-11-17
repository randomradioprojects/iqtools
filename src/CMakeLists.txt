cmake_minimum_required(VERSION 3.0)
project(iqtools)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

find_package(PkgConfig)
pkg_check_modules(VOLK REQUIRED volk)
link_directories(${VOLK_LIBRARY_DIRS})
include_directories(${VOLK_INCLUDE_DIRS})
link_libraries(${VOLK_LIBRARIES})

add_executable(iqtools_wavheader wavheader.cpp)
add_executable(iqtools_convert convert.cpp)
install(TARGETS iqtools_wavheader DESTINATION bin)
install(TARGETS iqtools_convert DESTINATION bin)
