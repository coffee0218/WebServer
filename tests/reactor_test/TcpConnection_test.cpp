#include "../../reactor/TcpServer.h"
#include "../../reactor/EventLoop.h"
#include "../../reactor/InetAddress.h"
#include <stdio.h>
/*
*本test的TcpConnection只处理了建立连接，它实际上是个diacard服务。但目前它永远
*不会关闭socket，即onConnection不会走到else分支
*/
void onConnection(const TcpConnectionPtr& conn)
{
  if (conn->connected())
  {
    printf("onConnection(): new connection [%s] from %s\n",
           conn->name().c_str(),
           conn->peerAddress().toHostPort().c_str());
  }
  else
  {
    printf("onConnection(): connection [%s] is down\n",
           conn->name().c_str());
  }
}

void onMessage(const TcpConnectionPtr& conn,
               const char* data,
               ssize_t len)
{
  printf("onMessage(): received %zd bytes from connection [%s]\n",
         len, conn->name().c_str());
}

int main()
{
  printf("main(): pid = %d\n", getpid());

  InetAddress listenAddr(9981);
  EventLoop loop;

  TcpServer server(&loop, listenAddr);
  server.setConnectioanCallback(onConnection);
  server.setMessageCallback(onMessage);
  server.start();

  loop.loop();
}
