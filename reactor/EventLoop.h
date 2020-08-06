#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include "../base/Thread.h"
#include <boost/scoped_ptr.hpp>
#include <vector>
#include "Channel.h"
#include "Poller.h"

class EventLoop : boost::noncopyable
{
 public:

  EventLoop();

  ~EventLoop();

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  ///
  void loop();

  void quit();

  // internal use only
  void updateChannel(Channel* channel);
  // void removeChannel(Channel* channel);

  void assertInLoopThread()
  {
    if (!isInLoopThread())
    {
      abortNotInLoopThread();
    }
  }

  bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

 private:

  void abortNotInLoopThread();

  typedef std::vector<Channel*> ChannelList;

  bool looping_; /* atomic */
  bool quit_; /* atomic */
  const pid_t threadId_;
  boost::scoped_ptr<Poller> poller_;//通过scoped_ptr来间接持有poller
  ChannelList activeChannels_;
};

#endif