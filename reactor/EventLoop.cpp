#include <assert.h>
#include <poll.h>

#include "../base/Logging.h"
#include "EventLoop.h"

__thread EventLoop* t_loopInThisThread = 0;

EventLoop::EventLoop()
  : looping_(false),
    threadId_(CurrentThread::tid())
{
  //LOG << "EventLoop created " << this << " in thread " << threadId_;
  if (t_loopInThisThread)
  {
    /*LOG << "Another EventLoop " << t_loopInThisThread
              << " exists in this thread " << threadId_;*/
  }
  else
  {
    t_loopInThisThread = this;
  }
}

EventLoop::~EventLoop()
{
  assert(!looping_);
  t_loopInThisThread = NULL;
}

void EventLoop::loop()
{
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;

  ::poll(NULL, 0, 5*1000);

  //LOG << "EventLoop " << this << " stop looping";
  looping_ = false;
}

void EventLoop::abortNotInLoopThread()
{
  /*LOG << "EventLoop::abortNotInLoopThread - EventLoop " << this
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();*/
}

