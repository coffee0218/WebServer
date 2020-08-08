#include "../base/Logging.h"
#include <sstream>
#include <poll.h>
#include "EventLoop.h"

using namespace std;

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;//读事件
const int Channel::kWriteEvent = POLLOUT;//写事件

Channel::Channel(EventLoop* loop, int fdArg)
  : loop_(loop),
    fd_(fdArg),
    events_(0),
    revents_(0),
    index_(-1)
{
}

//Channel::update()-->EventLoop::updateChannel-->Poller::updateChannel
//更新channel
void Channel::update()
{
  loop_->updateChannel(this);
}

//channel核心，它由EventLoop::loop()调用，它的功能是根据revents_的值分别调用不同的用户回调
void Channel::handleEvent()
{
  if (revents_ & POLLNVAL) {
    LOG << "Warning: Channel::handle_event() POLLNVAL";
  }

  if (revents_ & (POLLERR | POLLNVAL)) {
    if (errorCallback_) errorCallback_();
  }
  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (readCallback_) readCallback_();
  }
  if (revents_ & POLLOUT) {
    if (writeCallback_) writeCallback_();
  }
}