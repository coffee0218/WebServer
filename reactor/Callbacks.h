#pragma once

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "../base/Timestamp.h"

// All client visible callbacks go here.

class Buffer;
class TcpConnection;
typedef boost::shared_ptr<TcpConnection> TcpConnectionPtr;

typedef boost::function<void()> TimerCallback;
typedef boost::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef boost::function<void (const TcpConnectionPtr&,
                              Buffer* buf,
                              Timestamp)> MessageCallback;
typedef boost::function<void (const TcpConnectionPtr&)> CloseCallback;


