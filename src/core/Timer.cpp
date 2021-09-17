#include "Timer.h"

using namespace Hohnor;

std::atomic<uint64_t> Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
    if (interval_ > 0.0)
    {
        expiration_ = addTime(now, interval_);
    }
    else
    {
        expiration_ = Timestamp::invalid();
    }
}

TimerId Timer::id()
{
    return TimerId(this, sequence_);
}
