#include "AsyncLogging.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include "LogFile.h"

AsyncLogging::AsyncLogging(std::string logFileName_, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(logFileName_),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_(),
      latch_(1) {
  assert(logFileName_.size() > 1);
  currentBuffer_->bzero();
  nextBuffer_->bzero();
  buffers_.reserve(16);
}

void AsyncLogging::append(const char* logline, int len) {//前端发送方写日志
  MutexLockGuard lock(mutex_);
  if (currentBuffer_->avail() > len)
    currentBuffer_->append(logline, len);
  else {
    buffers_.push_back(currentBuffer_);
    currentBuffer_.reset();
    if (nextBuffer_)
      currentBuffer_ = std::move(nextBuffer_);//nextBuffer_可以减少前端临界区分配内存的概率，缩短前端的临界区
    else
      currentBuffer_.reset(new Buffer);//如果前端写入速度太快，给分配一块新的Buffer
    currentBuffer_->append(logline, len);
    cond_.notify();
  }
}

void AsyncLogging::threadFunc() {//后端接收日志
  assert(running_ == true);
  latch_.countDown();
  LogFile output(basename_);
  BufferPtr newBuffer1(new Buffer);//准备两块空闲的日志
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  while (running_) {
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      MutexLockGuard lock(mutex_);
      if (buffers_.empty())  // unusual usage!
      {
        cond_.waitForSeconds(flushInterval_);//触发条件：1，超时 2，前端写满了一个或者多个buffer
      }
      buffers_.push_back(currentBuffer_);
      currentBuffer_.reset();

      currentBuffer_ = std::move(newBuffer1);//将空闲的newBuffer1移为当前缓冲
      buffersToWrite.swap(buffers_);//内部指针交换而非复制，这样代码可以在临界区之外安全的访问buffersToWrite
      if (!nextBuffer_) {
        nextBuffer_ = std::move(newBuffer2);//neWBuffer2替换nextBuffer_，这样前端始终有一个预备buffer可供调配
      }
    }

    assert(!buffersToWrite.empty());

    if (buffersToWrite.size() > 25) {//处理过多的日志堆积问题，直接丢弃
     /* char buf[256];
      snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger buffers\n",
                Timestamp::now().toFormattedString().c_str(),
                buffersToWrite.size()-2);
      fputs(buf, stderr);
      output.append(buf, static_cast<int>(strlen(buf)));*/
      buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
    }

    for (size_t i = 0; i < buffersToWrite.size(); ++i) {
      output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());
    }

    if (buffersToWrite.size() > 2) {
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);
    }

    if (!newBuffer1) {
      assert(!buffersToWrite.empty());
      newBuffer1 = buffersToWrite.back();
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2) {
      assert(!buffersToWrite.empty());
      newBuffer2 = buffersToWrite.back();
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    buffersToWrite.clear();
    output.flush();
  }
  output.flush();
}
