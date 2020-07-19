#include "../ThreadPool.h"
#include "../CountDownLatch.h"
#include "../CurrentThread.h"
#include "../Logging.h"

#include <stdio.h>
#include <unistd.h>  // usleep

void print()
{
  printf("tid=%d\n", CurrentThread::tid());
}

void printString(const std::string& str)
{
  LOG << str;
  usleep(100*1000);
}

void test(int maxSize)
{
  LOG << "Test ThreadPool with max queue size = " << maxSize;
  ThreadPool pool("MainThreadPool");
  pool.setMaxQueueSize(maxSize);
  pool.start(5);

  LOG << "Adding";
  pool.run(print);
  pool.run(print);
  for (int i = 0; i < 100; ++i)
  {
    char buf[32];
    snprintf(buf, sizeof buf, "task %d", i);
    pool.run(std::bind(printString, std::string(buf)));
  }
  LOG << "Done";

  CountDownLatch latch(1);
  pool.run(std::bind(&CountDownLatch::countDown, &latch));
  latch.wait();
  pool.stop();
}

void longTask(int num)
{
  LOG << "longTask " << num;
  usleep(3000000);
}

void test2()
{
  LOG << "Test ThreadPool by stoping early.";
  ThreadPool pool("ThreadPool");
  pool.setMaxQueueSize(5);
  pool.start(3);

  Thread thread1([&pool]()
  {
    for (int i = 0; i < 20; ++i)
    {
      pool.run(std::bind(longTask, i));
    }
  }, "thread1");
  thread1.start();

  usleep(5000000);
  LOG << "stop pool";
  pool.stop();  // early stop

  thread1.join();
  // run() after stop()
  pool.run(print);
  LOG << "test2 Done";
}

int main()
{
  test(0);
  test(1);
  test(5);
  test(10);
  test(50);
  test2();
}
