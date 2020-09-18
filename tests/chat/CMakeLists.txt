add_executable(client client.cpp)
target_link_libraries(client libserver_reactor libserver_base)

add_executable(server server.cpp)
target_link_libraries(server libserver_reactor libserver_base)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin/chat)