#pragma once

#include "../../reactor/TcpServer.h"
#include "../../reactor/TcpConnection.h"

#include <unordered_set>

#include <boost/circular_buffer.hpp>

// RFC 862
class EchoServer
{
 public:
  EchoServer(EventLoop* loop,
             const InetAddress& listenAddr,
             int idleSeconds);

  void start();

 private:
  void onConnection(const TcpConnectionPtr& conn);

  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp time);

  void onTimer();

  void dumpConnectionBuckets() const;

  typedef boost::weak_ptr<TcpConnection> WeakTcpConnectionPtr;

  struct Entry : public copyable
  {
    explicit Entry(const WeakTcpConnectionPtr& weakConn)
      : weakConn_(weakConn)
    {
    }

    ~Entry()
    {
      TcpConnectionPtr conn = weakConn_.lock();
      if (conn)
      {
        conn->shutdown();
      }
    }

    WeakTcpConnectionPtr weakConn_;
  };
  typedef std::shared_ptr<Entry> EntryPtr;
  typedef std::weak_ptr<Entry> WeakEntryPtr;
  typedef std::unordered_set<EntryPtr> Bucket;
  typedef boost::circular_buffer<Bucket> WeakConnectionList;

  TcpServer server_;
  WeakConnectionList connectionBuckets_;
};

