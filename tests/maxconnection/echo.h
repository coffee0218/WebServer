#pragma once

#include "../../reactor/TcpServer.h"

class EchoServer
{
 public:
  EchoServer(EventLoop* loop,
             const InetAddress& listenAddr,
             int maxConnections);

  void start();

 private:
  void onConnection(const TcpConnectionPtr& conn);

  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp time);

  TcpServer server_;
  int numConnected_; // should be atomic_int
  const int kMaxConnections_;
};

