#pragma once

#include "../base/Thread.h"
#include <boost/scoped_ptr.hpp>
#include <vector>
#include "Channel.h"
#include "EPoller.h"
#include "Poller.h"
#include "Callbacks.h"
#include "TimerId.h"
#include "TimerQueue.h"
#include "../base/MutexLock.h"

class EventLoop : boost::noncopyable
{
 public:
  typedef boost::function<void()> Functor;
  EventLoop();

  ~EventLoop();

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  ///
  void loop();

  void quit();

  /* 
   * Runs callback immediately in the loop thread.It wakes up the loop, and run the cb.
   * If in the same loop thread, cb is run within the function. Safe to call from other threads.
   */
  void runInLoop(const Functor& cb);
  /*
  * Queues callback in the loop thread. Runs after finish pooling.
  * Safe to call from other threads.
  */
  void queueInLoop(const Functor& cb);

  /// Runs callback at 'time'.
  TimerId runAt(const Timestamp& time, const TimerCallback& cb);
  /// Runs callback after delay seconds.
  TimerId runAfter(double delay, const TimerCallback& cb);
  /// Runs callback every interval seconds.
  TimerId runEvery(double interval, const TimerCallback& cb);

  void cancel(TimerId timerId);

  // internal use only
  void wakeup();
  void updateChannel(Channel* channel);
  void removeChannel(Channel* channel);

  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }

  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

 private:

  void abortNotInLoopThread();
  void handleRead();  // waked up
  void doPendingFunctors();

  typedef std::vector<Channel*> ChannelList;

  bool looping_; /* atomic */
  bool quit_; /* atomic */
  bool callingPendingFunctors_; /* atomic */
  const pid_t threadId_;
  Timestamp pollReturnTime_;
  boost::scoped_ptr<EPoller> poller_;//通过scoped_ptr来间接持有poller
  boost::scoped_ptr<TimerQueue> timerQueue_;
  int wakeupFd_;
  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  boost::scoped_ptr<Channel> wakeupChannel_;//wakeupChannel_用于处理wakeupFd_上的readable事件，将事件分发至handleRead()函数
  ChannelList activeChannels_;
  MutexLock mutex_;
  std::vector<Functor> pendingFunctors_; //pendingFunctors_保存回调函数，暴露给了其他线程，因此用mutex保护
};
