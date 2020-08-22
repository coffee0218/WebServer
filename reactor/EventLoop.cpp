#include <assert.h>

#include "../base/Logging.h"
#include "EventLoop.h"
#include <sys/eventfd.h>
#include <boost/bind.hpp>
#include <signal.h>

using namespace std;

__thread EventLoop* t_loopInThisThread = 0;
//one loop per thread, 每个线程只能有一个EventLoop对象，t_loopInThisThread值是唯一的
const int kPollTimeMs = 10000;

/*
*由于IO线程平时阻塞在事件循环loop()的poll调用中，为了让IO线程能够立刻执行用户回调，
*我们需要设法唤醒它。传统方法是用pipe，IO线程始终监视此管道的readable事件，
*在需要唤醒的时候，其他线程往管道里写一个字节，这样IO线程就从IO multiplexing阻塞调用中返回。
*现在Linux有了eventfd，可以更高效地唤醒，因为它不必管理缓冲区。
*/
static int createEventfd()
{
  int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evtfd < 0)
  {
    LOG << "syserror: Failed in eventfd";
    abort();
  }
  return evtfd;
}

class IgnoreSigPipe
{
 public:
  IgnoreSigPipe()
  {
    ::signal(SIGPIPE, SIG_IGN);
  }
};

IgnoreSigPipe initObj;

EventLoop::EventLoop()
  : looping_(false),
    quit_(false),
    callingPendingFunctors_(false),
    threadId_(CurrentThread::tid()),
    poller_(new Poller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_))
{
  LOG << "trace: EventLoop created in thread" << threadId_;
  if (t_loopInThisThread)
  {
    LOG << "fatal error: Another EventLoop exists in this thread " << threadId_;
  }
  else
  {
    t_loopInThisThread = this;
  }

  wakeupChannel_->setReadCallback(
      boost::bind(&EventLoop::handleRead, this));
  // we are always reading the wakeupfd
  wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
  assert(!looping_);
  ::close(wakeupFd_);
  t_loopInThisThread = NULL;
}

TimerId EventLoop::runAt(const Timestamp& time, const TimerCallback& cb)
{
  return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), delay));
  return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const TimerCallback& cb)
{
  Timestamp time(addTime(Timestamp::now(), interval));
  return timerQueue_->addTimer(cb, time, interval);
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
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
    for (ChannelList::iterator it = activeChannels_.begin();
        it != activeChannels_.end(); ++it)
    {
      (*it)->handleEvent(pollReturnTime_);
    }
    doPendingFunctors();
  }

  LOG << "trace: EventLoop stop looping";
  looping_ = false;
}

//quit必要时唤醒IO线程，让它终止循环
void EventLoop::quit()
{
  quit_ = true;
  if (!isInLoopThread())
  {
    wakeup();
  }
}

/*
*如果用户是在当前IO线程调用这个函数，回调会同步进行；
*如果用户在其他吸纳从调用runInLoop(), cb会被加入队列，
*IO线程会被唤醒来调用这个Functor
*/
void EventLoop::runInLoop(const Functor& cb)
{
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(cb);
  }
}

/*
*由于IO线程平时阻塞在事件循环loop()的poll调用中，为了让IO线程能够立刻执行用户回调，
*我们需要设法唤醒它，queueInLoop将cb放入队列，并在必要时调用wakeup唤醒IO线程。
*必要时有两种情况：如果调用的不是IO线程，必须唤醒。
*如果在IO线程调用queueInLoop(),而此时正在调用pending functor, 也必须唤醒。
*只有在IO线程的回调事件回调中调用queueInLoop()才无需wakeup().
*/
void EventLoop::queueInLoop(const Functor& cb)
{
  {
    MutexLockGuard lock(mutex_);
    pendingFunctors_.push_back(cb);
  }

  if (!isInLoopThread() || callingPendingFunctors_)
  {
    wakeup();//向wakeupFd_写数据，唤醒它的读事件
  }
}

void EventLoop::updateChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel)
{
  assert(channel->ownerLoop() == this);
  assertInLoopThread();
  poller_->removeChannel(channel);
}

void EventLoop::abortNotInLoopThread()
{
  LOG << "fatal error: EventLoop::abortNotInLoopThread"
            << " was created in threadId_ = " << threadId_
            << ", current thread id = " <<  CurrentThread::tid();
}

void EventLoop::wakeup()
{
  uint64_t one = 1;
  ssize_t n = ::write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG<< "error: EventLoop::wakeup() writes " << n << " bytes instead of 8";
  }
}

void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = ::read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG << "error: EventLoop::handleRead() reads " << n << " bytes instead of 8";
  }
}

/*
*doPendingFunctors()把回调列表swap()到局部变量functors中，这样一方面减小了临界区长度
*(不会阻塞其他线程调用queueInLoop()),另一方面也避免了死锁(因为Functor可能会再调用queueInLoop())
*/
void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;
  callingPendingFunctors_ = true;

  {
    MutexLockGuard lock(mutex_);
    functors.swap(pendingFunctors_);
  }

  for (size_t i = 0; i < functors.size(); ++i)
  {
    functors[i]();
  }
  callingPendingFunctors_ = false;
}
