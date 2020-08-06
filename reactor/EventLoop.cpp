#include <assert.h>

#include "../base/Logging.h"
#include "EventLoop.h"

using namespace std;

__thread EventLoop* t_loopInThisThread = 0;
//one loop per thread, 每个线程只能有一个EventLoop对象，t_loopInThisThread值是唯一的
const int kPollTimeMs = 10000;

EventLoop::EventLoop()
  : looping_(false),
    quit_(false),
    threadId_(CurrentThread::tid()),
    poller_(new Poller())
{
  LOG << "EventLoop created in thread" << threadId_;
  if (t_loopInThisThread)
  {
    LOG << "fatal error: Another EventLoop exists in this thread " << threadId_;
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

//调用IO复用poll()获取当前活动事件的channel列表，然后依次调用每个channel的handleEvent()函数
void EventLoop::loop()
{
  assert(!looping_);
  assertInLoopThread();
  looping_ = true;
  quit_ = false;

  while (!quit_)
  {
    activeChannels_.clear();
    poller_->poll(kPollTimeMs, &activeChannels_);
    for (ChannelList::iterator it = activeChannels_.begin();
        it != activeChannels_.end(); ++it)
    {
      (*it)->handleEvent();
    }
  }

  LOG << "EventLoop stop looping";
  looping_ = false;
}

void EventLoop::quit()
{
  quit_ = true;
  // wakeup();
}

void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  LOG << "fatal error: EventLoop::abortNotInLoopThread"
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}

