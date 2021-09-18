#include "SignalHandlerSet.h"
#include "EventLoop.h"
#include "SignalUtils.h"
using namespace Hohnor;

SignalHandlerSet::SignalHandlerSet(EventLoop *loop) : loop_(loop), signalPipeHandler_(loop, SignalUtils::readEndFd()), sets_(new std::set<SignalHandler *>[64])
{
    signalPipeHandler_.setReadCallback(std::bind(&SignalHandlerSet::handleRead, this));
    signalPipeHandler_.enable();
}

SignalHandlerSet::~SignalHandlerSet()
{
    for (int i = 0; i < 64; i++)
    {
        auto &set = sets_[i];
        if (set.size())
        {
            SignalUtils::handleSignal(i + 1, SignalUtils::Default);
        }
        for (auto ptr : set)
        {
            delete ptr;
        }
    }
}

void SignalHandlerSet::handleRead()
{
    loop_->assertInLoopThread();
    auto it = SignalUtils::receive();
    while (it.hasNext())
    {
        char sig = it.next();
        for (auto ptr : sets_[sig - 1])
        {
            ptr->run();
        }
    }
}
