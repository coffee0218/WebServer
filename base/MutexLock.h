#pragma once
#include <pthread.h>
#include <cstdio>
#include "noncopyable.h"
#include "CurrentThread.h"
#include <assert.h>
#include <stdlib.h>

/* 由于pthread系列函数返回成功的时候都是0，因此，
我们可以写一个宏作为一个轻量级的检查手段，来判断处理错误。*/

#define MCHECK(exp) \
    if(exp) \
    { \
        fprintf(stderr, "File:%s, Line:%d Exp:[" #exp "] is true, abort.\n",__FILE__, __LINE__); abort();\
    }

class MutexLock : noncopyable {
 public:
  MutexLock() 
    : holder_(0)
  { 
    MCHECK(pthread_mutex_init(&mutex_, NULL)); 
  }

  ~MutexLock() {
    assert(holder_ == 0);
    MCHECK(pthread_mutex_destroy(&mutex_));
  }

  // must be called when locked, i.e. for assertion
  bool isLockedByThisThread() const
  {
    return holder_ == CurrentThread::tid();
  }

  void assertLocked() const
  {
    assert(isLockedByThisThread());
  }

  void lock() 
  { 
    MCHECK(pthread_mutex_lock(&mutex_));
    assignHolder(); 
  }

  void unlock() 
  { 
    unassignHolder();
    MCHECK(pthread_mutex_unlock(&mutex_)); 
  }

  pthread_mutex_t* getPthreadMutex() /* non-const */
  {
    return &mutex_;
  }

 private:
  pthread_mutex_t mutex_;
  pid_t holder_;

  void unassignHolder()
  {
    holder_ = 0;
  }

  void assignHolder()
  {
    holder_ = CurrentThread::tid();
  }
  
  friend class Condition;// 友元类不受访问权限影响
};

//使用RAII（资源获取即初始化）技术，对MutexLock初始化和析构进行处理。
//初始化的时候加锁，析构的时候解锁
class MutexLockGuard : noncopyable {
 public:
  explicit MutexLockGuard(MutexLock &_mutex) 
  : mutex(_mutex) 
  { 
    mutex.lock(); 
  }

  ~MutexLockGuard() 
  { 
    mutex.unlock();
  }

 private:
  MutexLock &mutex;
};