set(http_SRCS
  HttpServer.cpp
  HttpResponse.cpp
  HttpContext.cpp
  )

add_library(libserver_http ${http_SRCS})
target_link_libraries(libserver_http libserver_reactor libserver_base)

add_executable(HttpServer_test HttpServer_test.cpp)
target_link_libraries(HttpServer_test libserver_http)

set (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/bin/http)

set_target_properties(libserver_http PROPERTIES OUTPUT_NAME "server_http")
set (LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)