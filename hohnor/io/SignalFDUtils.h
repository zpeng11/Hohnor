/**
 * Handling signal to prevent interupt, send them to a socket pipe and allow epoll to handle
 */

#pragma once
#include <csignal>

namespace Hohnor
{
    namespace SignalUtils
    {
        enum SigAction
        {
            Ignored,
            Default,
            Handled //Return signal fd that handles the signal
        };
        //Handle a signal with callback function, thread safe
        int handleSignal(int signal, SigAction action = Handled);
    }
}