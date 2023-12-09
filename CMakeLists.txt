cmake_minimum_required(VERSION 3.26)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/winvxdlib/toolchain.cmake)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_COMPILE_WARNING_AS_ERROR ON)
set(CMAKE_C_FLAGS "-O3 -Wall")
set(CMAKE_CXX_FLAGS "-O3 -Wall")

project(hand386usb)

add_subdirectory(winvxdlib)

add_subdirectory(usbapi)
add_subdirectory(ch375)

add_subdirectory(ch375mse)
