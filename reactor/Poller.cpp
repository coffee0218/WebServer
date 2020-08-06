#include "../base/Logging.h"
#include "Poller.h"
#include <assert.h>
#include <poll.h>

using namespace std;
/*
*Poller是EventLoop的间接成员，只供其onwer EventLoop在IO线程调用，
*因此无需加锁，其生命周期和eventLoop相等
*/
Poller::Poller()
{
}

Poller::~Poller()
{
}

//获得当前活动的IO事件，然后填充调用方法传入的activeChannels
Timestamp Poller::poll(int timeoutMs, ChannelList* activeChannels)
{
  //int poll(struct pollfd *fds, nfds_t nfds, int timeout);
  int numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
  Timestamp now(Timestamp::now());
  if (numEvents > 0) {
    LOG << numEvents << " events happended";
    fillActiveChannels(numEvents, activeChannels);
  } else if (numEvents == 0) {
    LOG << " nothing happended";
  } else {
    LOG << "error: Poller::poll()";
  }
  return now;
}

/*fillActiveChannels遍历pollfds_,找到有活动事件的fd，把它对应的channel填入activeChannels
 *这个函数的时间复杂度是O(N),其中N是pollfds_的长度，即文件描述符数目。
 *numEvent减为0表示活动的fd都找完了，不必做无用功
 *当前活动事件都会保存在channel中，供handleEvent()使用
*/
void Poller::fillActiveChannels(int numEvents,
                                ChannelList* activeChannels) const
{
  for (PollFdList::const_iterator pfd = pollfds_.begin();
      pfd != pollfds_.end() && numEvents > 0; ++pfd)
  {
    if (pfd->revents > 0)
    {
      --numEvents;
      ChannelMap::const_iterator ch = channels_.find(pfd->fd);
      assert(ch != channels_.end());
      Channel* channel = ch->second;
      assert(channel->fd() == pfd->fd);
      channel->set_revents(pfd->revents);
      // pfd->revents = 0;
      activeChannels->push_back(channel);
    }
  }
}

/*Poller::updateChannel()的主要功能是负责维护和更新pollfds_数组，
*添加新channel的复杂度是O(NlogN),更新已有的channel的复杂度为O（1）
*/
void Poller::updateChannel(Channel* channel)
{
  //assertInLoopThread();
  LOG << "fd = " << channel->fd() << " events = " << channel->events();
  if (channel->index() < 0) {
    // a new one, add to pollfds_
    assert(channels_.find(channel->fd()) == channels_.end());
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);
    int idx = static_cast<int>(pollfds_.size())-1;
    channel->set_index(idx);
    channels_[pfd.fd] = channel;
  } else {
    // update existing one
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    struct pollfd& pfd = pollfds_[idx];
    assert(pfd.fd == channel->fd() || pfd.fd == -1);
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    if (channel->isNoneEvent()) {
      // ignore this pollfd
      pfd.fd = -1;
    }
  }
}

