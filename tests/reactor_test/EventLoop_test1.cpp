#include "../../reactor/EventLoop.h"
#include <stdio.h>

//在主线程和子线程中分别创建EventLoop, 程序正常运行退出
void threadFunc()
{
  printf("threadFunc(): pid = %d, tid = %d\n",
         getpid(), CurrentThread::tid());

  EventLoop loop;
  loop.loop();
}

int main()
{
  printf("main(): pid = %d, tid = %d\n",
         getpid(), CurrentThread::tid());

  EventLoop loop;

  Thread thread(threadFunc);
  thread.start();

  loop.loop();
  pthread_exit(NULL);
}
