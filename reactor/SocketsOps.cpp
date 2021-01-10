#include "SocketsOps.h"
#include "../base/Logging.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>  // snprintf
#include <strings.h>  // bzero
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

void setNonBlockAndCloseOnExec(int sockfd)
{
  // non-block
  int flags = ::fcntl(sockfd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(sockfd, F_SETFL, flags);
  if(ret < 0)
  {
    LOG << "system fatal: setNonBlockAndCloseOnExec";
  }

  // close-on-exec关闭子进程无用文件描述符
  flags = ::fcntl(sockfd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(sockfd, F_SETFD, flags);
  if(ret < 0)
  {
    LOG << "system fatal: setNonBlockAndCloseOnExec";
  }
}


int sockets::createNonblockingOrDie()
{
  // 创建socket
  int sockfd = ::socket(AF_INET,
                        SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                        IPPROTO_TCP);
  if (sockfd < 0)
  {
    LOG << "system fatal: sockets::createNonblockingOrDie";
  }
  return sockfd;
}

int sockets::connect(int sockfd, const struct sockaddr_in& addr)
{
  return ::connect(sockfd, (struct sockaddr *)(&addr), sizeof addr);
}

void sockets::bindOrDie(int sockfd, const struct sockaddr_in& addr)
{
  int ret = ::bind(sockfd, (struct sockaddr *)(&addr), sizeof addr);
  if (ret < 0)
  {
    LOG << "system fatal: sockets::bindOrDie";
  }
}

void sockets::listenOrDie(int sockfd)
{
  int ret = ::listen(sockfd, SOMAXCONN);
  if (ret < 0)
  {
    LOG << "system fatal: sockets::listenOrDie";
  }
}

int sockets::accept(int sockfd, struct sockaddr_in* addr)
{
  socklen_t addrlen = sizeof *addr;
  int connfd = ::accept4(sockfd, (struct sockaddr *)(addr),
                         &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd < 0)
  {
    int savedErrno = errno;
    LOG << "sys error: Socket::accept";
    switch (savedErrno)
    {
      //这里区分致命错误和暂时错误，并且区别对待
      case EAGAIN://暂时错误忽略
      case ECONNABORTED:
      case EINTR:
      case EPROTO:
      case EPERM:
      case EMFILE:
        errno = savedErrno;
        break;
      case EBADF://致命错误终止程序
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        // unexpected errors
        LOG << "fatal: unexpected error of ::accept " << savedErrno;
        abort();
        break;
      default:
        LOG << "fatal: unknown error of ::accept " << savedErrno;
        abort();
        break;
    }
  }
  return connfd;
}

void sockets::close(int sockfd)
{
  if (::close(sockfd) < 0)
  {
    LOG << "system error: sockets::close";
  }
}

void sockets::shutdownWrite(int sockfd)
{
  if (::shutdown(sockfd, SHUT_WR) < 0)
  {
    LOG << "system error: sockets::shutdownWrite";
  }
}

//网络字节序转换成主机字节序
void sockets::toHostPort(char* buf, size_t size,
                         const struct sockaddr_in& addr)
{
  char host[INET_ADDRSTRLEN] = "INVALID";
  ::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof host);
  uint16_t port = sockets::networkToHost16(addr.sin_port);
  snprintf(buf, size, "%s:%u", host, port);
}

//主机字节序转换成网络字节序
void sockets::fromHostPort(const char* ip, uint16_t port,
                           struct sockaddr_in* addr)
{
  addr->sin_family = AF_INET;
  addr->sin_port = hostToNetwork16(port);
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0)
  {
    LOG << "system error: sockets::fromHostPort";
  }
}

/*
* getsockname()函数用于获取一个套接字的名字。
*/
struct sockaddr_in sockets::getLocalAddr(int sockfd)
{
  struct sockaddr_in localaddr;
  bzero(&localaddr, sizeof localaddr);
  socklen_t addrlen = sizeof(localaddr);
  if (::getsockname(sockfd, (struct sockaddr *)(&localaddr), &addrlen) < 0)
  {
    LOG << "system error: sockets::getLocalAddr";
  }
  return localaddr;
}

struct sockaddr_in sockets::getPeerAddr(int sockfd)
{
  struct sockaddr_in peeraddr;
  bzero(&peeraddr, sizeof peeraddr);
  socklen_t addrlen = sizeof(peeraddr);
  if (::getpeername(sockfd, (struct sockaddr *)(&peeraddr), &addrlen) < 0)
  {
    LOG << "system error: sockets::getPeerAddr";
  }
  return peeraddr;
}

int sockets::getSocketError(int sockfd)
{
  int optval;
  socklen_t optlen = sizeof optval;

  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
  {
    return errno;
  }
  else
  {
    return optval;
  }
}

bool sockets::isSelfConnect(int sockfd)
{
  struct sockaddr_in localaddr = getLocalAddr(sockfd);
  struct sockaddr_in peeraddr = getPeerAddr(sockfd);
  return localaddr.sin_port == peeraddr.sin_port
      && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}
