cmake_minimum_required (VERSION 2.8)

project (WebServer)

option(MYDEBUG "enable test file compilation" ON)
add_compile_options(-std=c++11 -Wall -g) 

include_directories(${PROJECT_SOURCE_DIR}/base)
include_directories(${PROJECT_SOURCE_DIR}/reactor)

add_subdirectory (base)
add_subdirectory (reactor)
add_subdirectory (http)

if (MYDEBUG)
    add_subdirectory(tests)
else()
    message(STATUS "Currently don't compile tests file")    
endif()
