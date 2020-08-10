#include "Acceptor.h"

#include "../base/Logging.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>

using namespace std;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie()),//创建非阻塞的连接socket
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false)
{
  acceptSocket_.setReuseAddr(true);//设置端口复用
  acceptSocket_.bindAddress(listenAddr);//绑定
  acceptChannel_.setReadCallback(
      boost::bind(&Acceptor::handleRead, this));
}

void Acceptor::listen()//监听
{
  loop_->assertInLoopThread();
  listenning_ = true;
  acceptSocket_.listen();
  acceptChannel_.enableReading();
}
/*
*Channel用于观察此acceptSocket_上的readable事件，并回调Acceptor::handleRead()，
*后者会调用accept()来接受新连接，并回调用户callback newConnectionCallback_函数
*/
void Acceptor::handleRead()
{
  loop_->assertInLoopThread();
  InetAddress peerAddr(0);
  
  int connfd = acceptSocket_.accept(&peerAddr);
  if (connfd >= 0) {
    if (newConnectionCallback_) {
      newConnectionCallback_(connfd, peerAddr);
    } else {
      sockets::close(connfd);
    }
  }
}

