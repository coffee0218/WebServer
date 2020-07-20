link_directories (${PROJECT_SOURCE_DIR}/lib)

add_executable(test1 test1.cpp)
target_link_libraries(test1 libserver_reactor)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)