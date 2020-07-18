#pragma once
#include <pthread.h>
#include <cstdio>
#include "noncopyable.h"


class MutexLock : noncopyable {
 public:
  MutexLock() { pthread_mutex_init(&mutex, NULL); }
  ~MutexLock() {
    pthread_mutex_lock(&mutex);
    pthread_mutex_destroy(&mutex);
  }
  void lock() { pthread_mutex_lock(&mutex); }
  void unlock() { pthread_mutex_unlock(&mutex); }
  pthread_mutex_t *get() { return &mutex; }

 private:
  pthread_mutex_t mutex;

 private:
  friend class Condition;// 友元类不受访问权限影响
};

//使用RAII（资源获取即初始化）技术，对MutexLock初始化和析构进行处理。
//初始化的时候加锁，析构的时候解锁
class MutexLockGuard : noncopyable {
 public:
  explicit MutexLockGuard(MutexLock &_mutex) : mutex(_mutex) { mutex.lock(); }
  ~MutexLockGuard() { mutex.unlock(); }

 private:
  MutexLock &mutex;
};