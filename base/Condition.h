#pragma once
#include <errno.h>
#include <pthread.h>
#include <pthread.h>
#include <time.h>
#include <cstdint>
#include "MutexLock.h"
#include "noncopyable.h"

/* 我们在调用pthread_cond_wait的时候，必须先加锁，需要用到之前MutexLock的私用成员函数，
  这里已经在MutexLock.h中将class Condition设定为了MutexLock的一个友元，所以可以不受限制的访问
  MutexLock的私用成员函数*/
class Condition : noncopyable {
 public:
  explicit Condition(MutexLock &_mutex) : mutex(_mutex) {
    pthread_cond_init(&cond, NULL);
  }
  ~Condition() { pthread_cond_destroy(&cond); }
  void wait() { pthread_cond_wait(&cond, mutex.get()); }//封装pthread_cond_wait
  void notify() { pthread_cond_signal(&cond); }//封装pthread_cond_signal
  void notifyAll() { pthread_cond_broadcast(&cond); }//封装pthread_cond_broadcast
  bool waitForSeconds(int seconds) {
    struct timespec abstime;
    clock_gettime(CLOCK_REALTIME, &abstime);
    abstime.tv_sec += static_cast<time_t>(seconds);
    return ETIMEDOUT == pthread_cond_timedwait(&cond, mutex.get(), &abstime);
  }

 private:
  MutexLock &mutex;
  pthread_cond_t cond;
};