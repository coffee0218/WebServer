#pragma once
#include <functional>
#include <string>
#include <vector>
#include "CountDownLatch.h"
#include "LogStream.h"
#include "MutexLock.h"
#include "Thread.h"
#include "noncopyable.h"


class AsyncLogging : noncopyable {
 public:
  AsyncLogging(const std::string basename, int flushInterval = 2);
  ~AsyncLogging() {
    if (running_) stop();
  }
  void append(const char* logline, int len);

  void start() {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  void stop() {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:
  void threadFunc();
  typedef FixedBuffer<kLargeBuffer> Buffer;//日志缓冲区，其大小为4M，可以至少存放1000条日志消息
  typedef std::vector<std::shared_ptr<Buffer>> BufferVector;//前端待写入日志的缓冲vector
  typedef std::shared_ptr<Buffer> BufferPtr;//share指针自动管理Buffer对象
  const int flushInterval_;//强制写入日志间隔事件
  bool running_;
  std::string basename_;
  Thread thread_;
  MutexLock mutex_;
  Condition cond_;
  BufferPtr currentBuffer_;//当前缓冲
  BufferPtr nextBuffer_;//备用缓冲
  BufferVector buffers_;
  CountDownLatch latch_;
};