#include "Timer.h"

using namespace Hohnor;

std::atomic<uint64_t> Timer::s_numCreated_;

Timer::Timer(TimerCallback callback, Timestamp when, double interval) : callback_(callback),
                                                                        expiration_(when),
                                                                        interval_(interval),
                                                                        disabled_(false),
                                                                        sequence_(s_numCreated_++)
{
}

void Timer::run()
{
    if (!disabled_)
    {
        callback_();
    }
}

void Timer::disable()
{
    interval_ = 0.0;
    disabled_ = true;
}

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
