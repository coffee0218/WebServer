#include "TcpConnection.h"

#include "../base/Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"
#include <boost/bind.hpp>

#include <errno.h>
#include <stdio.h>

using namespace std;

TcpConnection::TcpConnection(EventLoop* loop,
                             const std::string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
  : loop_(loop),
    name_(nameArg),
    state_(kConnecting),
    socket_(new Socket(sockfd)),
    channel_(new Channel(loop, sockfd)),
    localAddr_(localAddr),
    peerAddr_(peerAddr)
{
  if(loop_ == NULL)
    LOG << "fatal error: TcpConnection::TcpConnection loop can't be NULL";
  LOG << "TcpConnection::ctor[" <<  name_ << "] at "
            << " fd=" << sockfd;
  channel_->setReadCallback(
      boost::bind(&TcpConnection::handleRead, this));
  channel_->setWriteCallback(
      boost::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(
      boost::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(
      boost::bind(&TcpConnection::handleError, this));
}

TcpConnection::~TcpConnection()
{
  LOG << "TcpConnection::dtor[" <<  name_ << "] at "
            << " fd=" << channel_->fd();
  printf("TcpConnection::~TcpConnection, TcpConnetion release");
}

void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

/*
*TcpConnection::connectDestroyed()是TcpConnection析构前最后调用的一个成员函数，它通知用户连接已断开
*/
void TcpConnection::connectDestroyed()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnected);
  setState(kDisconnected);
  channel_->disableAll();
  connectionCallback_(shared_from_this());

  loop_->removeChannel(get_pointer(channel_));//EventLoop新增了removeChannel()成员函数，它会调用Poller::removeChannel()
}

/*
*TcpConnection::handleRead()会检查read的返回值，根据返回值分别调用
*messageCallback_，handleClose()，handleError()
*/
void TcpConnection::handleRead()
{
  char buf[65536];
  ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
  if (n > 0) {
    messageCallback_(shared_from_this(), buf, n);
  } else if (n == 0) {
    handleClose();
  } else {
    handleError();
  }
}

void TcpConnection::handleWrite()
{
}

/*
*TcpConnection增加了CloseCallback事件回调，但是这个回调时给TcpServer和TcpClient用的，用于通知移除它们所持有的TcpConnectionPtr，
*这不是给普通用户用的，普通用户继续使用ConnectionCallback
*TcpConnection::handleClose()的主要功能是调用closeCallback_，这个会调绑定到TcpServer::removeConnection
*/
void TcpConnection::handleClose()
{
  loop_->assertInLoopThread();
  LOG << "trace: TcpConnection::handleClose state = " << state_;
  assert(state_ == kConnected);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  channel_->disableAll();
  // must be the last line
  closeCallback_(shared_from_this());
}

void TcpConnection::handleError()
{
  int err = sockets::getSocketError(channel_->fd());
  LOG << "error: TcpConnection::handleError [" << name_
            << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}