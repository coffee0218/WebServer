#include "Timer.h"

void Timer::restart(Timestamp now)//设置Timer的下一次过期时间
{
  if (repeat_)
  {
    expiration_ = addTime(now, interval_);
  }
  else
  {
    expiration_ = Timestamp::invalid();
  }
}
