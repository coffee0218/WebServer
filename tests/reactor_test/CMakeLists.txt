add_executable(EventLoop_test1 EventLoop_test1.cpp)
target_link_libraries(EventLoop_test1 libserver_reactor)

add_executable(EventLoop_test2 EventLoop_test2.cpp)
target_link_libraries(EventLoop_test2 libserver_reactor)

add_executable(EventLoop_test3 EventLoop_test3.cpp)
target_link_libraries(EventLoop_test3 libserver_reactor)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)
