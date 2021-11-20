#include "SignalSet.h"
#include "EventLoop.h"
#include "SignalUtils.h"
using namespace Hohnor;

SignalHandlerSet::SignalHandlerSet(EventLoop *loop) : loop_(loop), signalPipeHandler_(loop, SignalUtils::readEndFd()), sets_(new std::set<Signal *>[64])
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
        auto v = sets_[sig - 1];
        for (auto ptr : v)
        {
            ptr->run(loop_);
        }
    }
}

void SignalHandlerSet::add(char signal, SignalCallback cb)
{
    Signal *handler = new Signal(signal, std::move(cb));
    loop_->runInLoop(std::bind(&SignalHandlerSet::addInLoop, this, handler));
}

void SignalHandlerSet::remove(Signal* signal)
{
    loop_->runInLoop(std::bind(&SignalHandlerSet::removeInLoop, this, signal));
}

void SignalHandlerSet::addInLoop(Signal *handler)
{
    loop_->assertInLoopThread();
    sets_[handler->signal() - 1].insert(handler);
    SignalUtils::handleSignal(handler->signal(), SignalUtils::Piped);
}

void SignalHandlerSet::removeInLoop(Signal* signal)
{
    loop_->assertInLoopThread();
    auto it = sets_[signal->signal()-1].find(signal);
    if(it != sets_[signal->signal()-1].end())
    {
        sets_[signal->signal()-1].erase(it);
    }
    else{
        LOG_WARN<<"There is not this signal handler";
    }
    if(!sets_[signal->signal()-1].size())
    {
        SignalUtils::handleSignal(signal->signal(), SignalUtils::Default);
    }
}