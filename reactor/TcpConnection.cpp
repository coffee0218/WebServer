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
      boost::bind(&TcpConnection::handleRead, this, _1));
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

void TcpConnection::send(const std::string& message)
{
  if (state_ == kConnected) {
    if (loop_->isInLoopThread()) {
      sendInLoop(message);
    } else {
      loop_->runInLoop(
          boost::bind(&TcpConnection::sendInLoop, this, message));
    }
  }
}

void TcpConnection::send(Buffer* buf)
{
  if (state_ == kConnected)
  {
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf->retrieveAllAsString());
    }
    else
    {
      loop_->runInLoop(
          boost::bind(&TcpConnection::sendInLoop, this, buf->retrieveAllAsString()));
    }
  }
}

/*
*sendInLoop会先尝试直接发送数据，如果一次发送完毕就不会启用WriteCallback。
*如果只发送了部分数据，则把剩余的数据放入outputBuffer_, 并开始关注writable事件，
*以后在handleWrite()中发送剩余的数据。
*/
void TcpConnection::sendInLoop(const std::string& message)
{
  loop_->assertInLoopThread();
  ssize_t nwrote = 0;
  // if no thing in output queue, try writing directly
  if (!channel_->isWriting() && outputBuffer_.readableBytes() == 0) {
    nwrote = ::write(channel_->fd(), message.data(), message.size());
    if (nwrote >= 0) {
      if (static_cast<size_t>(nwrote) < message.size()) {
        LOG<< "trace: I am going to write more data";
      }
    }  
    else if (writeCompleteCallback_) {
        loop_->queueInLoop(
            boost::bind(writeCompleteCallback_, shared_from_this()));
    }
    else {
      nwrote = 0;
      if (errno != EWOULDBLOCK) {//EWOULDBLOCK用于非阻塞模式，不需要重新读或者写, EWOULDBLOCK = EAGAIN
        LOG<< "system error: TcpConnection::sendInLoop";
      }
    }
  }

  assert(nwrote >= 0);
  if (static_cast<size_t>(nwrote) < message.size()) {
    outputBuffer_.append(message.data()+nwrote, message.size()-nwrote);
    if (!channel_->isWriting()) {
      channel_->enableWriting();
    }
  }
}

void TcpConnection::shutdown()
{
  // FIXME: use compare and swap
  if (state_ == kConnected)
  {
    setState(kDisconnecting);
    // FIXME: shared_from_this()?
    loop_->runInLoop(boost::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop()
{
  loop_->assertInLoopThread();
  if (!channel_->isWriting())
  {
    // we are not writing
    socket_->shutdownWrite();
  }
}

void TcpConnection::setTcpNoDelay(bool on)
{
  socket_->setTcpNoDelay(on);
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
  assert(state_ == kConnected || state_ == kDisconnecting);
  setState(kDisconnected);
  channel_->disableAll();
  connectionCallback_(shared_from_this());

  loop_->removeChannel(get_pointer(channel_));//EventLoop新增了removeChannel()成员函数，它会调用Poller::removeChannel()
}

/*
*TcpConnection::handleRead()会检查read的返回值，根据返回值分别调用
*messageCallback_，handleClose()，handleError()
*/
void TcpConnection::handleRead(Timestamp receiveTime)
{
  int savedErrno = 0;
  ssize_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
  if (n > 0) {
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  } else if (n == 0) {
    handleClose();
  } else {
    errno = savedErrno;
    LOG << "system error: TcpConnection::handleRead";
    handleError();
  }
}

void TcpConnection::handleWrite()
{
  loop_->assertInLoopThread();
  if (channel_->isWriting()) {
    ssize_t n = ::write(channel_->fd(),
                        outputBuffer_.peek(),
                        outputBuffer_.readableBytes());
    if (n > 0) {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readableBytes() == 0) 
      {
        channel_->disableWriting();
        if (writeCompleteCallback_) {
          loop_->queueInLoop(
              boost::bind(writeCompleteCallback_, shared_from_this()));
        }
        if (state_ == kDisconnecting) 
        {
          shutdownInLoop();
        }
      } 
      else 
      {
        LOG << "trace: I am going to write more data";
      }
    } 
    else 
    {
      LOG << "system error: TcpConnection::handleWrite";
    }
  } 
  else 
  {
    LOG << "trace: Connection is down, no more writing";
  }
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
  assert(state_ == kConnected || state_ == kDisconnecting);
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