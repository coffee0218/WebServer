#include "../../reactor/EventLoop.h"
#include "../../reactor/EventLoopThread.h"
#include <stdio.h>
//测试EventLoopThread的功能
void runInThread()
{
  printf("runInThread(): pid = %d, tid = %d\n",
         getpid(), CurrentThread::tid());
}

int main()
{
  printf("main(): pid = %d, tid = %d\n",
         getpid(), CurrentThread::tid());

  EventLoopThread loopThread;
  EventLoop* loop = loopThread.startLoop();//loop是在另一个线程创建
  loop->runInLoop(runInThread);
  sleep(1);
  loop->runAfter(2, runInThread);
  sleep(3);
  loop->quit();

  printf("exit main().\n");
}

/*
*main(): pid = 97516, tid = 97516
*runInThread(): pid = 97516, tid = 97517
*runInThread(): pid = 97516, tid = 97517
*exit main().
*/