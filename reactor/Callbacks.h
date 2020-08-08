#pragma once

#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "../base/Timestamp.h"

// All client visible callbacks go here.
typedef boost::function<void()> TimerCallback;

