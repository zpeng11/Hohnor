/**
 * Define for callback function types
 */

#pragma once
#include <functional>

namespace Hohnor
{
    class Timestamp;
    typedef std::function<void()> Callback;
    typedef std::function<void()> Functor;
    typedef std::function<void()> ReadCallback;
    typedef std::function<void()> WriteCallback;
    typedef std::function<void()> CloseCallback;
    typedef std::function<void()> ErrorCallback;
    typedef std::function<void(int)> SignalCallback;
    typedef std::function<void(Timestamp)> TimerCallback;

} // namespace Hohnor
