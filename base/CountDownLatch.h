#pragma once

#include "Condition.h"
#include "MutexLock.h"

/*
 * CountDownLatch,本质上来说,是一个thread safe的计数器,用于主线程和工作线程的同步.
 * 
 * 用法：
 * 第一种:在初始化时,需要指定主线程需要等待的任务的个数(count),当工作线程完成 Task Callback后对计数器减1，
 * 而主线程通过wait()调用阻塞等待技术器减到0为止.
 * 
 * 第二种:初始化计数器值为1,在程序结尾将创建一个线程执行countDown操作并wait()当程序执行到最后会阻塞直到计数器减为0,
 * 这可以保证线程池中的线程都start了线程池对象才完成析够
*/ 
class CountDownLatch : noncopyable
{
 public:

  explicit CountDownLatch(int count);

  void wait();

  void countDown();

  int getCount() const;

 private:
  mutable MutexLock mutex_;
  Condition condition_;
  int count_;
};

