#include "ThreadPool.h"

#include <assert.h>
#include <stdio.h>

ThreadPool::ThreadPool(const string& nameArg)
  : mutex_(),
    notEmpty_(mutex_),
    notFull_(mutex_),
    name_(nameArg),
    maxQueueSize_(0),
    running_(false)
{
}

ThreadPool::~ThreadPool()
{
  if (running_)
  {
    stop();
  }
}

void ThreadPool::start(int numThreads)
{
  assert(threads_.empty());
  running_ = true;
  threads_.reserve(numThreads);
  for (int i = 0; i < numThreads; ++i)
  {
    char id[32];
    snprintf(id, sizeof id, "%d", i+1);
    threads_.emplace_back(new Thread(
          std::bind(&ThreadPool::runInThread, this), name_+id));//将runInThread绑定到thread，注册回调函数
    threads_[i]->start();//启动线程，调用回调函数runInThread，也就是处理task
  }
  if (numThreads == 0 && threadInitCallback_)
  {
    threadInitCallback_();
  }
}

/*
 * stop 线程池过程
 * 先将running_设置为false，然后调用notifyAll，激活所有等待的线程,
 * 关闭所有的条件等待,线程退出.
 */
void ThreadPool::stop()
{
  {
  MutexLockGuard lock(mutex_);
  running_ = false;
  notEmpty_.notifyAll();////激活所有的线程
  notFull_.notifyAll();
  }
  for (auto& thr : threads_)
  {
    thr->join();//回收停止运行的线程
  }
}

size_t ThreadPool::queueSize() const
{
  MutexLockGuard lock(mutex_);
  return queue_.size();
}

void ThreadPool::run(Task task)//添加任务
{
  if (threads_.empty())
  {
    task();
  }
  else
  {
    MutexLockGuard lock(mutex_);
    while (isFull() && running_)
    {
      notFull_.wait();
    }
    if (!running_) return;//这里注意，退出while循环后，需要再判断一次bool变量，
    //因为未必是条件满足了，可能是线程池需要退出，调整了running_变量
    assert(!isFull());

    queue_.push_back(std::move(task));
    notEmpty_.notify();
  }
}

ThreadPool::Task ThreadPool:: take()//取走任务
{
  MutexLockGuard lock(mutex_);
  // always use a while-loop, due to spurious wakeup
  while (queue_.empty() && running_)
  {
    notEmpty_.wait();
  }
  Task task;
  if (!queue_.empty())
  {
    task = queue_.front();
    queue_.pop_front();
    if (maxQueueSize_ > 0)
    {
      notFull_.notify();
    }
  }
  return task;
}

bool ThreadPool:: isFull() const
{
  mutex_.assertLocked();
  return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()//注册在Thread的回调函数，作用是从queue中take task
{
  try
  {
    if (threadInitCallback_)
    {
      threadInitCallback_();
    }
    while (running_)
    {
      Task task(take());//阻塞直到有task添加到队列
      if (task)
      {
        task();
      }
    }
  }
  catch (...)
  {
    fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
    throw; // rethrow
  }
}

