cmake_minimum_required(VERSION 3.5)
project(iqtools)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

find_package(PkgConfig)
pkg_check_modules(VOLK REQUIRED volk)
link_directories(${VOLK_LIBRARY_DIRS})
include_directories(${VOLK_INCLUDE_DIRS})
link_libraries(${VOLK_LIBRARIES})

add_executable(iqtools-wavheader wavheader.cpp)
add_executable(iqtools-wavinfo wavinfo.cpp)
add_executable(iqtools-convert convert.cpp)
install(TARGETS iqtools-wavheader DESTINATION bin)
install(TARGETS iqtools-wavinfo DESTINATION bin)
install(TARGETS iqtools-convert DESTINATION bin)
