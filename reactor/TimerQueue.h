#pragma once

#include <set>
#include <vector>

#include <boost/noncopyable.hpp>

#include "../base/Timestamp.h"
#include "../base/MutexLock.h"
#include "Callbacks.h"
#include "Channel.h"

class EventLoop;
class Timer;
class TimerId;

class TimerQueue : boost::noncopyable
{
 public:
  TimerQueue(EventLoop* loop);
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  TimerId addTimer(const TimerCallback& cb,
                   Timestamp when,
                   double interval);

  // void cancel(TimerId timerId);

 private:

  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TimerList;//TimerList是set而非map，因为只有key没有value

  void handleRead();// 定时器到期，timerfd_上有可读事件事件时被调用
  
  std::vector<Entry> getExpired(Timestamp now);// move out all expired timers
  void reset(const std::vector<Entry>& expired, Timestamp now);

  bool insert(Timer* timer);

  EventLoop* loop_;
  const int timerfd_;//使用一个timerfdChannel_来观察timerfd_上的readable事件
  Channel timerfdChannel_;
  // Timer list sorted by expiration
  TimerList timers_;
};

/*TimerQueue需要高效地组织目前尚未到期的Timer，能快速根据当前时间找到已经到期的Timer，也能高效地添加和删除Timer。
* 使用二叉搜索树（set或者map），把Timer按到期时间先后排好序，操作复杂度仍然是O(NlogN)
* 但是我们不能直接用map<Timestamp, Timer*>，因为这样无法处理两个Timer到期时间相同的情况。
* 采用pair< Timestamp, Timer*>为key，这样即使两个Timer的到期时间相同，它们的地址也必定不同
*/