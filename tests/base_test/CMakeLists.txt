add_executable(Thread_test Thread_test.cpp)
target_link_libraries(Thread_test libserver_base)

add_executable(Thread_bench Thread_bench.cpp)
target_link_libraries(Thread_bench libserver_base)

add_executable(ThreadPool_test ThreadPool_test.cpp)
target_link_libraries(ThreadPool_test libserver_base)

add_executable(Logging_test Logging_test.cpp)
target_link_libraries(Logging_test libserver_base)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin)
