#pragma once

#include "../base/copyable.h"

class Timer;

///
/// An opaque identifier, for canceling Timer.
///
class TimerId : public copyable
{
 public:
  TimerId(Timer* timer = NULL, int64_t seq = 0)
    : timer_(timer),
      sequence_(seq)
  {
  }

  friend class TimerQueue;

 private:
  Timer* timer_;
  int64_t sequence_;
};

