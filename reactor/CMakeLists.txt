set(LIB_SRC
    Acceptor.cpp
    Channel.cpp
    EventLoop.cpp
    EventLoopThread.cpp
    InetAddress.cpp
    Poller.cpp
    Socket.cpp
    SocketsOps.cpp
    TcpConnection.cpp
    TcpServer.cpp
    Timer.cpp
    TimerQueue.cpp
)

add_library(libserver_reactor ${LIB_SRC})
target_link_libraries(libserver_reactor libserver_base)

set_target_properties(libserver_reactor PROPERTIES OUTPUT_NAME "server_reactor")
set (LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build/lib)
