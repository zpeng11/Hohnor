#include "Signal.h"
#include "EventLoop.h"

using namespace Hohnor;

std::atomic<uint64_t> Signal::s_numCreated_;


void Signal::run(EventLoop * loop)
{
    callback_(signal_, SignalHandle(this,loop));
}

void SignalHandle::cancel()
{
    loop_->removeSignal(* this);
}