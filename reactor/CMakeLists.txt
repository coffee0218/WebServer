set(LIB_SRC
    EventLoop.cpp
)
include_directories(${PROJECT_SOURCE_DIR}/base)

add_library(libserver_reactor ${LIB_SRC})
target_link_libraries(libserver_reactor libserver_base)

set_target_properties(libserver_reactor PROPERTIES OUTPUT_NAME "server_reactor")
set (LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

if (MYDEBUG)
    add_subdirectory(tests)
else()
    message(STATUS "Currently don't compile tests file")    
endif()