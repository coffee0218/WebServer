set(LIB_SRC
    AsyncLogging.cpp
    CountDownLatch.cpp
    FileUtil.cpp
    LogFile.cpp
    Logging.cpp
    LogStream.cpp
    Thread.cpp
)

add_library(libserver_base ${LIB_SRC})
target_link_libraries(libserver_base pthread rt)
set_target_properties(libserver_base PROPERTIES OUTPUT_NAME "server_base")
set (LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

if (MYDEBUG)
    add_subdirectory(tests)
else()
    message(STATUS "Currently don't compile tests file")    
endif()

