add_executable(main main.cpp echo.cpp)
target_link_libraries(main libserver_reactor libserver_base)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin/idleconnection)