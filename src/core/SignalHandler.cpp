#include "SignalHandler.h"

using namespace Hohnor;

std::atomic<uint64_t> SignalHandler::s_numCreated_;

SignalHandlerId SignalHandler::id()
{
    return SignalHandlerId(this, sequence_);
}