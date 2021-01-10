#pragma once

#include <map>
#include <vector>
#include "../base/Timestamp.h"

class channel;
class EventLoop;

struct pollfd;

///
/// IO Multiplexing with poll(2).
///
/// This class doesn't own the Channel objects.
class Poller : boost::noncopyable
{
 public:
  typedef std::vector<Channel*> ChannelList;

  Poller(EventLoop* loop);
  ~Poller();//析构简单，因为它的成员变量都是标准库容器

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  Timestamp poll(int timeoutMs, ChannelList* activeChannels);

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  void updateChannel(Channel* channel);
  /// Remove the channel, when it destructs.
  /// Must be called in the loop thread.
  void removeChannel(Channel* channel);

  void assertInLoopThread();

 private:
  void fillActiveChannels(int numEvents,
                          ChannelList* activeChannels) const;

  typedef std::vector<struct pollfd> PollFdList;
  typedef std::map<int, Channel*> ChannelMap;

  EventLoop* ownerLoop_;
  PollFdList pollfds_;//pollfd数组
  ChannelMap channels_;
};


