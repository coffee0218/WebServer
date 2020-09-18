#pragma once

#include "../../base/Logging.h"
#include "../../reactor/Buffer.h"
#include "../../reactor/SocketsOps.h"
#include "../../reactor/TcpConnection.h"
#include <boost/bind.hpp>

class LengthHeaderCodec : noncopyable
{
 public:
  typedef boost::function<void (const TcpConnectionPtr&,
                                const string& message,
                                Timestamp)> StringMessageCallback;

  explicit LengthHeaderCodec(const StringMessageCallback& cb)
    : messageCallback_(cb)
  {
  }

  void onMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receiveTime)
  {
    while (buf->readableBytes() >= kHeaderLen) // kHeaderLen == 4
    {
      // FIXME: use Buffer::peekInt32()
      const void* data = buf->peek();
      int32_t be32 = *static_cast<const int32_t*>(data); // SIGBUS
      const int32_t len = sockets::networkToHost32(be32);
      if (len > 65536 || len < 0)
      {
        LOG << "error: Invalid length " << len;
        conn->shutdown();  // FIXME: disable reading
        break;
      }
      else if (buf->readableBytes() >= len + kHeaderLen)
      {
        buf->retrieve(kHeaderLen);
        string message(buf->peek(), len);
        messageCallback_(conn, message, receiveTime);
        buf->retrieve(len);
      }
      else
      {
        break;
      }
    }
  }

  // FIXME: TcpConnectionPtr
  void send(TcpConnection* conn,
            const string& message)
  {
    Buffer buf;
    buf.append(message);
    int32_t len = static_cast<int32_t>(message.size());
    int32_t be32 = sockets::hostToNetwork32(len);
    buf.prepend(&be32, sizeof be32);
    conn->send(&buf);
  }

 private:
  StringMessageCallback messageCallback_;
  const static size_t kHeaderLen = sizeof(int32_t);
};

