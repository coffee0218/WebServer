# A C++ High Performance Web Server

## Introduction  
本项目为C++11编写的网络库，最终实现高并发Web服务器，解析了客户端的http请求，可处理静态资源，支持HTTP长连接，并实现了异步日志，记录服务器运行状态。开发高并发Web服务器能够很好地贯穿C++后台开发所需要的知识，包括C++11语法和编程规范，网络编程，线程同步，IO多路复用，线程通信，TCP/HTTP协议，Linux系统和CMake编译等知识。

## Environment

+ 操作系统: Ubuntu 18.04

+ 编译器: gcc version 7.5.0

+ 版本控制: git

+ 自动化构建: cmake

+ 压测工具：[WebBench](https://github.com/EZLippi/WebBench)

## Build

```
mkdir build
cd build/
cmake .. && make 
```
## Technology points

•	使用epoll多路复用技术实现高并发处理请求，使用 Reactor 编程模型
•	状态机解析 HTTP 请求，目前支持 HTTP GET、HEAD 方法
•	为减少内存泄漏的可能，使用智能指针等 RAII 机制
•	使用双缓冲区技术实现了简单的异步日志系统
•	使用timerfd，用处理IO事件相同的方式来处理定时
•	使用多线程充分利用多核 CPU，并使用线程池避免线程频繁创建销毁的开销
•	主线程只负责accept新请求，并以Round Robin的方式分发给其它IO线程(兼计算线程，线程已提前创建好)，锁的争用只会出现在主线程和某一特定线程中 
•	使用eventfd实现了线程的异步唤醒

