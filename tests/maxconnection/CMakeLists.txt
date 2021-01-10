add_executable(EchoServer main.cpp echo.cpp)
target_link_libraries(EchoServer libserver_reactor libserver_base)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin/maxconnection)
