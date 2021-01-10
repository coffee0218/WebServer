#include "echo.h"

#include "../../base/Logging.h"
#include "../../reactor/EventLoop.h"

#include <assert.h>
#include <stdio.h>
#include <boost/bind.hpp>

using namespace std;

EchoServer::EchoServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       int idleSeconds)
  : server_(loop, listenAddr),
    connectionBuckets_(idleSeconds)
{
  server_.setConnectionCallback(
      boost::bind(&EchoServer::onConnection, this, _1));
  server_.setMessageCallback(
      boost::bind(&EchoServer::onMessage, this, _1, _2, _3));
  loop->runEvery(1.0, boost::bind(&EchoServer::onTimer, this));
  connectionBuckets_.resize(idleSeconds);
  dumpConnectionBuckets();
}

void EchoServer::start()
{
  server_.start();
}

void EchoServer::onConnection(const TcpConnectionPtr& conn)
{
  LOG << "EchoServer - " << conn->peerAddress().toHostPort() << " -> "
           << conn->localAddress().toHostPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");
  printf("EchoServer: new connection [%s] peer is %s %s\n",
           conn->name().c_str(),
           conn->peerAddress().toHostPort().c_str(),
           conn->connected() ? "UP" : "DOWN");

  if (conn->connected())
  {
    EntryPtr entry(new Entry(conn));
    connectionBuckets_.back().insert(entry);
    dumpConnectionBuckets();
    WeakEntryPtr weakEntry(entry);
    conn->setContext(weakEntry);
  }
  else
  {
    assert(!conn->getContext().empty());
    WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
    LOG << "Entry use_count = " << weakEntry.use_count();
  }
}

void EchoServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp time)
{
  string msg(buf->retrieveAllAsString());
  LOG << conn->name() << " echo " << msg.size()
           << " bytes at " << time.toString();
  printf("%s echo %ld bytes at %s\n", conn->name().c_str(), msg.size(), time.toString().c_str());
  conn->send(msg);

  assert(!conn->getContext().empty());
  WeakEntryPtr weakEntry(boost::any_cast<WeakEntryPtr>(conn->getContext()));
  EntryPtr entry(weakEntry.lock());
  if (entry)
  {
    connectionBuckets_.back().insert(entry);
    dumpConnectionBuckets();
  }
}

void EchoServer::onTimer()
{
  connectionBuckets_.push_back(Bucket());
  dumpConnectionBuckets();
}

void EchoServer::dumpConnectionBuckets() const
{
  LOG << "size = " << connectionBuckets_.size();

  int idx = 0;
  for (WeakConnectionList::const_iterator bucketI = connectionBuckets_.begin();
      bucketI != connectionBuckets_.end();
      ++bucketI, ++idx)
  {
    const Bucket& bucket = *bucketI;
    //printf("[%d] len = %zd : ", idx, bucket.size());
    for (const auto& it : bucket)
    {
      bool connectionDead = it->weakConn_.expired();
      printf("%p(%ld)%s, ", get_pointer(it), it.use_count(),
          connectionDead ? " DEAD" : "");
    }
    puts("");
  }
}

