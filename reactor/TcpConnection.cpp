#include "TcpConnection.h"

#include "../base/Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"

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
}

TcpConnection::~TcpConnection()
{
  LOG << "TcpConnection::dtor[" <<  name_ << "] at "
            << " fd=" << channel_->fd();
}

void TcpConnection::connectEstablished()
{
  loop_->assertInLoopThread();
  assert(state_ == kConnecting);
  setState(kConnected);
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

void TcpConnection::handleRead()
{
  char buf[65536];
  ssize_t n = ::read(channel_->fd(), buf, sizeof buf);
  messageCallback_(shared_from_this(), buf, n);
  // FIXME: close connection if n == 0
}

