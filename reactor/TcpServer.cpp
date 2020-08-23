#include "TcpServer.h"

#include "../base/Logging.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>

#include <stdio.h>

using namespace std;

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    name_(listenAddr.toHostPort()),
    acceptor_(new Acceptor(loop, listenAddr)),
    threadPool_(new EventLoopThreadPool(loop)),
    started_(false),
    nextConnId_(1)
{
  if(loop_ == NULL)
    LOG << "fatal error: TcpServer::TcpServer loop can't be NULL";
  acceptor_->setNewConnectionCallback(
      boost::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
}

void TcpServer::setThreadNum(int numThreads)
{
  assert(0 <= numThreads);
  threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
  if (!started_)
  {
    started_ = true;
  }

  if (!acceptor_->listenning())
  {
    loop_->runInLoop(
        boost::bind(&Acceptor::listen, get_pointer(acceptor_)));
  }
}

/*
*在新连接到达时，Acceptor会回调newConnection()，后者会创建TcpConnection对象conn，把它加入到ConnectionMap，设置好callback，
*再调用conn->connectEstablished()，其中会回调用户提供的ConnectionCallback
*/
void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
  loop_->assertInLoopThread();
  char buf[32];
  snprintf(buf, sizeof buf, "#%d", nextConnId_);
  ++nextConnId_;
  std::string connName = name_ + buf;

  LOG << "TcpServer::newConnection [" << name_
           << "] - new connection [" << connName
           << "] from " << peerAddr.toHostPort();
  InetAddress localAddr(sockets::getLocalAddr(sockfd));//sockfd是accept之后返回的连接fd
  // FIXME poll with zero timeout to double confirm the new connection
  EventLoop* ioLoop = threadPool_->getNextLoop();
  TcpConnectionPtr conn(
      new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
  connections_[connName] = conn;//每个TcpConnection对象有一个名字，这个名字是由其所属的TcpServer在创建TcpConnection对象时生成，名字是ConnectionMap的key。
  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      boost::bind(&TcpServer::removeConnection, this, _1));//TcpServer向TcpConnection注册CloseCallback，用于接收连接断开的消息
  ioLoop->runInLoop(boost::bind(&TcpConnection::connectEstablished, conn));
}

/*
*TcpServer::removeConnection把conn从ConnectionMap中移除。这时TcpConnection已经是命悬一线：
*如果用户不持有TcpConnectionPtr的话，conn的引用计数已降到1。注意这里一定要用EventLoop::queueInLoop()，否则会出现生命管理问题。
*另外注意这里用boost::bind让TcpConnection的生命期延长到connectDestoryed()的时刻
*/
void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
  // FIXME: unsafe
  loop_->runInLoop(boost::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  LOG << "info: TcpServer::removeConnectionInLoop [" << name_
           << "] - connection " << conn->name();
  size_t n = connections_.erase(conn->name());
  assert(n == 1); (void)n;
  EventLoop* ioLoop = conn->getLoop();
  ioLoop->queueInLoop(
      boost::bind(&TcpConnection::connectDestroyed, conn));
}