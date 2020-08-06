#include "../../reactor/EventLoop.h"
#include "../../base/Thread.h"

EventLoop* g_loop;
//在主线程创建一个EventLoop对象，却在子线程中调用loop(),程序会因断言失败终止
void threadFunc()
{
  g_loop->loop();
}

int main()
{
  EventLoop loop;
  g_loop = &loop;
  Thread t(threadFunc);
  t.start();
  t.join();
}
