#include "echo.h"

#include "../../base/Logging.h"
#include "../../reactor/EventLoop.h"

#include <unistd.h>

using namespace std;

int main(int argc, char* argv[])
{
  LOG << "pid = " << getpid();
  EventLoop loop;
  InetAddress listenAddr(2007);
  int maxConnections = 5;
  if (argc > 1)
  {
    maxConnections = atoi(argv[1]);
  }
  LOG << "maxConnections = " << maxConnections;
  EchoServer server(&loop, listenAddr, maxConnections);
  server.start();
  loop.loop();
}

