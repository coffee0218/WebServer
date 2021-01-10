#include "Buffer.h"
#include "SocketsOps.h"
#include "../base/Logging.h"

#include <errno.h>
#include <memory.h>
#include <sys/uio.h>

using namespace std;

const char Buffer::kCRLF[] = "\r\n";
/*
*在非阻塞网络编程中，如何设计并使用缓冲区？一方面我们希望减少系统调用，一次读的数据越多越划算，那么似乎应该准备一个大的缓冲区。
*另一方面，我们系统减少内存占用。如果有 10k 个连接，每个连接一建立就分配 64k 的读缓冲的话，将占用 640M 内存，
*而大多数时候这些缓冲区的使用率很低。Buffer用 readv 结合栈上空间巧妙地解决了这个问题。
*
*buffer默认会有一个1KB大小的vector，除此之外，在栈上申请一个65536字节大小的临时空间，
*这是当网络套接字数据太大时，可以将超出1K的数据先放入这个临时空间，然后在append到Buffer里面。
*结合栈上的空间，避免内存使用过大，提高内存使用率
*
*readv则将从fd读入的数据按同样的顺序散布到各缓冲区中，readv总是先填满一个缓冲区，然后再填下一个，避免了多次系统调用
*/
ssize_t Buffer::readFd(int fd, int* savedErrno)
{
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = writableBytes();
  vec[0].iov_base = begin()+writerIndex_;// 第一块缓冲区
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;// 第二块缓冲区
  vec[1].iov_len = sizeof extrabuf;
  const ssize_t n = readv(fd, vec, 2);
  if (n < 0) {
    *savedErrno = errno;
  } else if (static_cast<size_t>(n) <= writable) { //第一块缓冲区足够容纳
    writerIndex_ += n;
  } else {// 当前缓冲区，不够容纳，因而数据被接收到了第二块缓冲区extrabuf，将其append至buffer
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writable);
  }
  return n;
}

