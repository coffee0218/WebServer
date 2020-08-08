#include "../../reactor/Channel.h"
#include "../../reactor/EventLoop.h"

#include <stdio.h>
#include <sys/timerfd.h>

EventLoop* g_loop;

/*用timerfd实现了一个单次触发的定时器
*这个程序利用channel将timerfd的readable事件转发给timeout()函数
*/
void timeout()
{
  printf("Timeout!\n");
  g_loop->quit();
}

int main()
{
  EventLoop loop;
  g_loop = &loop;

  int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
  Channel channel(&loop, timerfd);
  channel.setReadCallback(timeout);
  channel.enableReading();

  struct itimerspec howlong;
  bzero(&howlong, sizeof howlong);
  howlong.it_value.tv_sec = 5;
  ::timerfd_settime(timerfd, 0, &howlong, NULL);

  loop.loop();

  ::close(timerfd);
}