#include "EventLoopThread.h"

#include "EventLoop.h"

#include <boost/bind.hpp>

using namespace std;

EventLoopThread::EventLoopThread()
  : loop_(NULL),
    exiting_(false),
    thread_(boost::bind(&EventLoopThread::threadFunc, this)),
    mutex_(),
    cond_(mutex_)
{
}

EventLoopThread::~EventLoopThread()
{
  exiting_ = true;
  loop_->quit();
  thread_.join();
}

/*
*EventLoopThread会启动自己的线程，并在其中运行loop()
*/
EventLoop* EventLoopThread::startLoop()
{
  assert(!thread_.started());
  thread_.start();

  {
    MutexLockGuard lock(mutex_);
    while (loop_ == NULL)
    {
      cond_.wait();
    }
  }

  return loop_;
}

/*
*线程主函数在stack上定义EventLoop对象，然后将其地址赋给loop_成员变量，
*最后调用条件变量的notify，唤醒startLoop()，成功创建EventLoop对象，并调用loop()
*/
void EventLoopThread::threadFunc()
{
  EventLoop loop;

  {
    MutexLockGuard lock(mutex_);
    loop_ = &loop;
    cond_.notify();
  }

  loop.loop();
  //assert(exiting_);
}

