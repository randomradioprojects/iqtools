cmake_minimum_required(VERSION 3.0)
project(iqtools)

set(CMAKE_INSTALL_PREFIX /usr CACHE PATH "Set cmake Default to /usr" FORCE)

add_subdirectory(src)

# Create uninstall target
configure_file(${CMAKE_SOURCE_DIR}/cmake_uninstall.cmake ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake @ONLY)
add_custom_target(uninstall ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
