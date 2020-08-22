#pragma once

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

#include "Channel.h"
#include "Socket.h"

class EventLoop;
class InetAddress;

///
/// Acceptor of incoming TCP connections.
/// 用于accept新Tcp连接，并通过回调通知使用者。它供Tcpserver使用，生命周期由后者控制。
class Acceptor : boost::noncopyable
{
 public:
  typedef boost::function<void (int sockfd,
                                const InetAddress&)> NewConnectionCallback;

  Acceptor(EventLoop* loop, const InetAddress& listenAddr);
  ~Acceptor();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  bool listenning() const { return listenning_; }
  void listen();

 private:
  void handleRead();

  EventLoop* loop_;
  Socket acceptSocket_;//Acceptor的socket是listening socket，即server socket
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;//用户的回调函数
  bool listenning_;
  int idleFd_;
};

