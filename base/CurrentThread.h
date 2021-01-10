#pragma once
#include <stdint.h>

/*
*__thread是GCC内置的线程局部存储设施，存取效率可以和全局变量相比。__thread变量每一个线程有一份独立实体，各个线程的值互不干扰。
*可以用来修饰那些带有全局性且值可能变，但是又不值得用全局变量保护的变量。
*
* CurrentThread::tid采取的办法是用__thread 变量来缓存gettid的返回值，这样只有在本线程第一次调用的时候才进行系统调用，
* 以后都是直接从 thread local缓存的线程id拿到结果，效率无忧。
*/
namespace CurrentThread {
  // internal
  extern __thread int t_cachedTid;
  extern __thread char t_tidString[32];
  extern __thread int t_tidStringLength;
  extern __thread const char* t_threadName;
  void cacheTid();
  inline int tid() {
    if (__builtin_expect(t_cachedTid == 0, 0)) {
      cacheTid();
    }
    return t_cachedTid;
  }

  inline const char* tidString()  // for logging
  {
    return t_tidString;
  }

  inline int tidStringLength()  // for logging
  {
    return t_tidStringLength;
  }

  inline const char* name() { return t_threadName; }
}
