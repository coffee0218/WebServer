#include "Acceptor.h"

#include "../base/Logging.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

#include <boost/bind.hpp>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
  : loop_(loop),
    acceptSocket_(sockets::createNonblockingOrDie()),//创建非阻塞的连接socket
    acceptChannel_(loop, acceptSocket_.fd()),
    listenning_(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC))
{
  assert(idleFd_ >= 0);
  acceptSocket_.setReuseAddr(true);//设置端口复用
  acceptSocket_.bindAddress(listenAddr);//绑定
  acceptChannel_.setReadCallback(
      boost::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor()
{
  ::close(idleFd_);
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
  else
  {
    LOG<< "system error: in Acceptor::handleRead";
    /*限制并发连接数，准备一个空闲的文件描述符。遇到文件描述符达到上限的情况，先关闭这个空闲文件，获得一个文件描述符的名额；
    *再accept拿到新socket连接的描述符，随后立即close它，这样就优雅地断开了客户端连接；最后重新打开一个空闲文件，把坑站占住。
    */
    if (errno == EMFILE)
    {
      ::close(idleFd_);
      idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL);
      ::close(idleFd_);
      idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

