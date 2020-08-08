#pragma once

#include <boost/noncopyable.hpp>

#include "../base/Timestamp.h"
#include "Callbacks.h"

class Timer : boost::noncopyable
{
 public:
  Timer(const TimerCallback& cb, Timestamp when, double interval)
    : callback_(cb),
      expiration_(when),
      interval_(interval),
      repeat_(interval > 0.0)
  { }

  void run() const//执行Timer的回调函数
  {
    callback_();
  }

  Timestamp expiration() const  { return expiration_; }
  bool repeat() const { return repeat_; }

  void restart(Timestamp now);

 private:
  const TimerCallback callback_;
  Timestamp expiration_;//过期时间
  const double interval_;//间隔
  const bool repeat_;//是否重复，interval_大于0就重复
};

