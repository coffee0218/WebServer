#include "codec.h"

#include "../../base/Logging.h"
#include "../../base/MutexLock.h"
#include "../../reactor/EventLoopThread.h"
#include "../../reactor/TcpClient.h"

#include <iostream>
#include <stdio.h>
#include <unistd.h>

using namespace std;

class ChatClient : noncopyable
{
 public:
  ChatClient(EventLoop* loop, const InetAddress& serverAddr)
    : client_(loop, serverAddr),
      codec_(boost::bind(&ChatClient::onStringMessage, this, _1, _2, _3))
  {
    client_.setConnectionCallback(
        boost::bind(&ChatClient::onConnection, this, _1));
    client_.setMessageCallback(
        boost::bind(&LengthHeaderCodec::onMessage, &codec_, _1, _2, _3));
    client_.enableRetry();
  }

  void connect()
  {
    client_.connect();
  }

  void disconnect()
  {
    client_.disconnect();
  }

  void write(const string& message)
  {
    MutexLockGuard lock(mutex_);
    if (connection_)
    {
      codec_.send(get_pointer(connection_), message);
    }
  }

 private:
  void onConnection(const TcpConnectionPtr& conn)
  {
    printf("onConnection(): tid=%d new connection [%s] from %s\n",
           CurrentThread::tid(),
           conn->name().c_str(),
           conn->peerAddress().toHostPort().c_str());

    MutexLockGuard lock(mutex_);
    if (conn->connected())
    {
      connection_ = conn;
    }
    else
    {
      connection_.reset();
    }
  }

  void onStringMessage(const TcpConnectionPtr&,
                       const string& message,
                       Timestamp)
  {
    printf("<<< %s\n", message.c_str());
  }

  TcpClient client_;
  LengthHeaderCodec codec_;
  MutexLock mutex_;
  TcpConnectionPtr connection_;
};

int main(int argc, char* argv[])
{
  LOG << "pid = " << getpid();
  if (argc > 2)
  {
    EventLoopThread loopThread;
    uint16_t port = static_cast<uint16_t>(atoi(argv[2]));
    InetAddress serverAddr(argv[1], port);

    ChatClient client(loopThread.startLoop(), serverAddr);
    client.connect();
    std::string line;
    while (std::getline(std::cin, line))
    {
      client.write(line);
    }
    client.disconnect();
    usleep(1000*1000);  // wait for disconnect
  }
  else
  {
    printf("Usage: %s host_ip port\n", argv[0]);
  }
}

