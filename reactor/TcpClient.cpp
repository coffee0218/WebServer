
#include "TcpClient.h"

#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include "../base/Logging.h"

#include <boost/bind.hpp>

#include <stdio.h>  // snprintf

using namespace std;

// TcpClient::TcpClient(EventLoop* loop)
//   : loop_(loop)
// {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//   : loop_(CHECK_NOTNULL(loop)),
//     serverAddr_(host, port)
// {
// }

namespace detail
{
  void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
  {
    loop->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
  }

  void removeConnector(const ConnectorPtr& connector)
  {
    //connector->
  }
}

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr)
  : loop_(loop),
    connector_(new Connector(loop, serverAddr)),
    retry_(false),
    connect_(true),
    nextConnId_(1)
{
  if(loop == NULL)
    LOG << "error: TcpClient::TcpClient loop can't be NULL";
  connector_->setNewConnectionCallback(
      boost::bind(&TcpClient::newConnection, this, _1));
  // FIXME setConnectFailedCallback
  LOG << "info: TcpClient::TcpClient - connector ";
}

TcpClient::~TcpClient()
{
  LOG << "info: TcpClient::~TcpClient - connector ";
  TcpConnectionPtr conn;
  {
    MutexLockGuard lock(mutex_);
    conn = connection_;
  }
  if (conn)
  {
    // FIXME: not 100% safe, if we are in different thread
    CloseCallback cb = boost::bind(&detail::removeConnection, loop_, _1);
    loop_->runInLoop(
        boost::bind(&TcpConnection::setCloseCallback, conn, cb));
  }
  else
  {
    connector_->stop();
    // FIXME: HACK
    loop_->runAfter(1, boost::bind(&detail::removeConnector, connector_));
  }
}

void TcpClient::connect()
{
  // FIXME: check state
  LOG << "info: TcpClient::connect - connecting to "
           << connector_->serverAddress().toHostPort();
  connect_ = true;
  connector_->start();
}

void TcpClient::disconnect()
{
  connect_ = false;

  {
    MutexLockGuard lock(mutex_);
    if (connection_)
    {
      connection_->shutdown();
    }
  }
}

void TcpClient::stop()
{
  connect_ = false;
  connector_->stop();
}

void TcpClient::newConnection(int sockfd)
{
  loop_->assertInLoopThread();
  InetAddress peerAddr(sockets::getPeerAddr(sockfd));
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toHostPort().c_str(), nextConnId_);
  ++nextConnId_;
  string connName = buf;

  InetAddress localAddr(sockets::getLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(loop_,
                                          connName,
                                          sockfd,
                                          localAddr,
                                          peerAddr));

  conn->setConnectionCallback(connectionCallback_);
  conn->setMessageCallback(messageCallback_);
  conn->setWriteCompleteCallback(writeCompleteCallback_);
  conn->setCloseCallback(
      boost::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
  {
    MutexLockGuard lock(mutex_);
    connection_ = conn;
  }
  conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
  loop_->assertInLoopThread();
  assert(loop_ == conn->getLoop());

  {
    MutexLockGuard lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  loop_->queueInLoop(boost::bind(&TcpConnection::connectDestroyed, conn));
  if (retry_ && connect_)
  {
    LOG << "info: TcpClient::connect - Reconnecting to "
             << connector_->serverAddress().toHostPort();
    connector_->restart();
  }
}

