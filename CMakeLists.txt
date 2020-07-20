cmake_minimum_required (VERSION 2.8)

project (WebServer)

option(MYDEBUG "enable test file compilation" ON)
add_compile_options(-std=c++11 -Wall -g) 

add_subdirectory (base)
add_subdirectory (reactor)
