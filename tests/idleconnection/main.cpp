#include "echo.h"
#include <stdio.h>

#include "../../base/Logging.h"
#include "../../reactor/EventLoop.h"

using namespace std;

int main(int argc, char* argv[])
{
  EventLoop loop;
  InetAddress listenAddr(2007);
  int idleSeconds = 10;
  if (argc > 1)
  {
    idleSeconds = atoi(argv[1]);
  }
  LOG << "pid = " << getpid() << ", idle seconds = " << idleSeconds;
  EchoServer server(&loop, listenAddr, idleSeconds);
  server.start();
  loop.loop();
}

