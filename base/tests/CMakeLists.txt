link_directories (${PROJECT_SOURCE_DIR}/lib)

add_executable(Thread_test Thread_test.cpp)
target_link_libraries(Thread_test libserver_base)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
