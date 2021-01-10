#pragma once

#include "Condition.h"
#include "MutexLock.h"
#include "Thread.h"

#include <deque>
#include <vector>
#include <string>

using namespace std;

class ThreadPool : noncopyable
{
 public:
  typedef std::function<void ()> Task;//我们使用了function，作为任务队列的任务元素

  explicit ThreadPool(const string& nameArg = string("ThreadPool"));
  ~ThreadPool();

  // Must be called before start().
  void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }//设置任务队列的大小
  void setThreadInitCallback(const Task& cb)
  { threadInitCallback_ = cb; }

  void start(int numThreads);
  void stop();

  const string& name() const
  { return name_; }

  size_t queueSize() const;

  // Could block if maxQueueSize > 0
  // Call after stop() will return immediately.
  // There is no move-only version of std::function in C++ as of C++14.
  // So we don't need to overload a const& and an && versions
  // as we do in (Bounded)BlockingQueue.
  void run(Task f);

 private:
  bool isFull() const;
  void runInThread();
  Task take();

  mutable MutexLock mutex_;//ThreadPool需要一把互斥锁和两个同步变量，实现同步与互斥
  Condition notEmpty_ ;
  Condition notFull_ ;
  string name_;
  Task threadInitCallback_;
  std::vector<std::unique_ptr<Thread>> threads_;
  std::deque<Task> queue_ ;
  size_t maxQueueSize_;
  bool running_;//判断线程池是否在工作
};



