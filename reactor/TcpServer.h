#pragma once

#include "Callbacks.h"
#include "TcpConnection.h"

#include <map>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>

class Acceptor;
class EventLoop;

/*
*TCP Server class的功能是管理accept获得的TcpConnection。TcpServer是供用户直接使用的，生命周期由用户控制。
*TcpServer的接口如下，用户只需要设置好callback，再调用start()即可
*
*TcpServer内部使用Acceptor来获得新连接的fd。它保存用户提供的ConnectionCallback和MessageCallback，
*在新建TcpConnection的时候会原样传给后者。
*/
class TcpServer : boost::noncopyable
{
 public:

  TcpServer(EventLoop* loop, const InetAddress& listenAddr);
  ~TcpServer();  // force out-line dtor, for scoped_ptr members.

  /// Starts the server if it's not listenning.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  void start();

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(const ConnectionCallback& cb)
  { connectionCallback_ = cb; }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(const MessageCallback& cb)
  { messageCallback_ = cb; }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(const WriteCompleteCallback& cb)
  { writeCompleteCallback_ = cb; }

 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd, const InetAddress& peerAddr);
  void removeConnection(const TcpConnectionPtr& conn);

  typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

  EventLoop* loop_;  // the acceptor loop
  const std::string name_;
  boost::scoped_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  bool started_;
  int nextConnId_;  // always in loop thread
  ConnectionMap connections_;
};

