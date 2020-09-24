#include "echo.h"
#include <boost/bind.hpp>

#include "../../base/Logging.h"

using namespace std;

EchoServer::EchoServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       int maxConnections)
  : server_(loop, listenAddr),
    numConnected_(0),
    kMaxConnections_(maxConnections)
{
  server_.setConnectionCallback(
      boost::bind(&EchoServer::onConnection, this, _1));
  server_.setMessageCallback(
      boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
}

void EchoServer::start()
{
  server_.start();
}

void EchoServer::onConnection(const TcpConnectionPtr& conn)
{
  LOG << "EchoServer - " << conn->peerAddress().toHostPort() << " -> "
           << conn->localAddress().toHostPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
  printf("EchoServer: new connection [%s] peer is %s %s\n",
           conn->name().c_str(),
           conn->peerAddress().toHostPort().c_str(),
           conn->connected() ? "UP" : "DOWN");

  if (conn->connected())
  {
    ++numConnected_;
    if (numConnected_ > kMaxConnections_)
    {
      conn->shutdown();
      printf("connections exceed max number\n");
    }
  }
  else
  {
    --numConnected_;
  }
  LOG << "numConnected = " << numConnected_;
}

void EchoServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp time)
{
  string msg(buf->retrieveAllAsString());
  LOG << conn->name() << " echo " << msg.size() << " bytes at " << time.toString();
  conn->send(msg);
}

