//此程序测试runInLoop()和queueInLoop()等函数
#include "../../reactor/EventLoop.h"
#include <stdio.h>

EventLoop* g_loop;
int g_flag = 0;

void run4()
{
  printf("run4(): pid = %d, flag = %d\n", getpid(), g_flag);
  g_loop->quit();
}

void run3()
{
  printf("run3(): pid = %d, flag = %d\n", getpid(), g_flag);
  g_loop->runAfter(3, run4);
  g_flag = 3;
}

void run2()
{
  printf("run2(): pid = %d, flag = %d\n", getpid(), g_flag);
  g_loop->queueInLoop(run3);//在run1执行完回调函数后，在 doPendingFunctors()中调用run3
}

void run1()
{
  g_flag = 1;
  printf("run1(): pid = %d, flag = %d\n", getpid(), g_flag);
  g_loop->runInLoop(run2);//run2是在IO线程执行，因此直接调用回调函数run2,flag == 1
  g_flag = 2;
}

int main()
{
  printf("main(): pid = %d, flag = %d\n", getpid(), g_flag);

  EventLoop loop;
  g_loop = &loop;

  loop.runAfter(2, run1);
  loop.loop();
  printf("main(): pid = %d, flag = %d\n", getpid(), g_flag);
}

/*
*都是再同一个IO线程里
*main(): pid = 96425, flag = 0
*run1(): pid = 96425, flag = 1
*run2(): pid = 96425, flag = 1
*run3(): pid = 96425, flag = 2
*run4(): pid = 96425, flag = 3
*main(): pid = 96425, flag = 3
*/