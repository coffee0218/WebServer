#pragma once

#include "../base/copyable.h"

#include <algorithm>
#include <string>
#include <vector>

#include <assert.h>

/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer : public copyable
{
 public:
  static const size_t kCheapPrepend = 8;//初始化prepend为8个字节大小
  static const size_t kInitialSize = 1024;//初始化Buffer大小为1024字节 默认

  Buffer()
    : buffer_(kCheapPrepend + kInitialSize),
      readerIndex_(kCheapPrepend),
      writerIndex_(kCheapPrepend)
  {
    assert(readableBytes() == 0);
    assert(writableBytes() == kInitialSize);
    assert(prependableBytes() == kCheapPrepend);
  }

  // default copy-ctor, dtor and assignment are fine

  void swap(Buffer& rhs)
  {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
  }

  size_t readableBytes() const//计算可读
  { return writerIndex_ - readerIndex_; }

  size_t writableBytes() const//计算可写
  { return buffer_.size() - writerIndex_; }

  size_t prependableBytes() const
  { return readerIndex_; }

  const char* peek() const//用来返回数据内容的起始位置
  { return begin() + readerIndex_; }

  // retrieve returns void, to prevent
  // string str(retrieve(readableBytes()), readableBytes());
  // the evaluation of two functions are unspecified
  void retrieve(size_t len)//该函数用来回收Buffer空间，在读取Buffer的内容后,调用此函数来挪动索引
  {
    assert(len <= readableBytes());
    readerIndex_ += len;
  }

  void retrieveUntil(const char* end)
  {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }

  void retrieveAll()//回收所有Buffer，将两个索引回归到初始位置
  {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }
  
  std::string retrieveAllAsString()//读取buffer所有的内容
  {
    std::string str(peek(), readableBytes());
    retrieveAll();
    return str;
  }

  void append(const std::string& str)
  {
    append(str.data(), str.length());
  }

  void append(const char* /*restrict*/ data, size_t len)//写入数据
  {
    ensureWritableBytes(len);//保证Buffer有足够空间可写
    std::copy(data, data+len, beginWrite());
    hasWritten(len);
  }

  void append(const void* /*restrict*/ data, size_t len)
  {
    append(static_cast<const char*>(data), len);
  }

  void ensureWritableBytes(size_t len)//对buffer进行扩容
  {
    if (writableBytes() < len)
    {
      makeSpace(len);
    }
    assert(writableBytes() >= len);
  }

  char* beginWrite()
  { return begin() + writerIndex_; }

  const char* beginWrite() const
  { return begin() + writerIndex_; }

  void hasWritten(size_t len)
  { writerIndex_ += len; }

  void prepend(const void* /*restrict*/ data, size_t len)
  {
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d+len, begin()+readerIndex_);
  }

  void shrink(size_t reserve)
  {
   std::vector<char> buf(kCheapPrepend+readableBytes()+reserve);
   std::copy(peek(), peek()+readableBytes(), buf.begin()+kCheapPrepend);
   buf.swap(buffer_);
  }

  /// Read data directly into buffer.
  ///
  /// It may implement with readv(2)
  /// @return result of read(2), @c errno is saved
  ssize_t readFd(int fd, int* savedErrno);

 private:

  char* begin()
  { return &*buffer_.begin(); }

  const char* begin() const
  { return &*buffer_.begin(); }

  void makeSpace(size_t len)//进行内部腾挪
  {
    if (writableBytes() + prependableBytes() < len + kCheapPrepend)
    {
      buffer_.resize(writerIndex_+len);//resize，这个时候内存不够了
    }
    else
    {
      // move readable data to the front, make space inside buffer
      assert(kCheapPrepend < readerIndex_);
      size_t readable = readableBytes();
      std::copy(begin()+readerIndex_,
                begin()+writerIndex_,
                begin()+kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
      assert(readable == readableBytes());
    }
  }

 private:
  std::vector<char> buffer_;//底层存储
  size_t readerIndex_;//读写索引
  size_t writerIndex_;
};

