/**
 * Define for callback function types
 */

#pragma once
#include <functional>

namespace Hohnor
{
    class Timestamp;
    class Timer;
    class SignalHandle;
    typedef std::function<void()> Callback;
    typedef std::function<void()> Functor;
    typedef std::function<void()> ReadCallback;
    typedef std::function<void()> WriteCallback;
    typedef std::function<void()> CloseCallback;
    typedef std::function<void()> ErrorCallback;
    typedef std::function<void(int, SignalHandle)> SignalCallback;
    typedef std::function<void(Timestamp, std::shared_ptr<Timer>)> TimerCallback;

} // namespace Hohnor
