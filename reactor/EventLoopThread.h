#pragma once

#include "../base/Condition.h"
#include "../base/MutexLock.h"
#include "../base/Thread.h"

#include <boost/noncopyable.hpp>

class EventLoop;

/*
*IO线程不一定是主线程，我们可以在任何一个线程创建并运行EventLoop。
*一个程序也可以有不止一个IO线程，我们可以按优先级将不同的socket分给不同的IO线程，避免优先级反转。
*定义EventLoopThread class正是one loop per thread的本意
*/
class EventLoopThread : boost::noncopyable
{
 public:
  EventLoopThread();
  ~EventLoopThread();
  EventLoop* startLoop();

 private:
  void threadFunc();

  EventLoop* loop_;
  bool exiting_;
  Thread thread_;
  MutexLock mutex_;
  Condition cond_;
};

