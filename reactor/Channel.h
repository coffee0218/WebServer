#ifndef CHANNEL_H
#define CHANNEL_H

#include <boost/function.hpp>
#include <boost/noncopyable.hpp>

class EventLoop;

/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel : boost::noncopyable
{
 public:
  typedef boost::function<void()> EventCallback;//回调函数类型

  Channel(EventLoop* loop, int fd);

  void handleEvent();
  void setReadCallback(const EventCallback& cb)//设置读写错误回调函数
  { readCallback_ = cb; }
  void setWriteCallback(const EventCallback& cb)
  { writeCallback_ = cb; }
  void setErrorCallback(const EventCallback& cb)
  { errorCallback_ = cb; }

  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revt) { revents_ = revt; }
  bool isNoneEvent() const { return events_ == kNoneEvent; }

  void enableReading() { events_ |= kReadEvent; update(); }
  // void enableWriting() { events_ |= kWriteEvent; update(); }
  // void disableWriting() { events_ &= ~kWriteEvent; update(); }
  // void disableAll() { events_ = kNoneEvent; update(); }

  // for Poller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  EventLoop* ownerLoop() { return loop_; }//channel属于哪个EventLoop

 private:
  void update();

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* loop_;//每个channel对象只属于一个EventLoop，也只属于一个IO线程
  const int  fd_;//每个channel对象只负责一个fd
  int        events_;//关心的IO事件，由用户设置
  int        revents_;//revents_是目前的活动事件，由EventLoop/Poller设置
  int        index_; // used by Poller.

  EventCallback readCallback_;
  EventCallback writeCallback_;
  EventCallback errorCallback_;
};
#endif
