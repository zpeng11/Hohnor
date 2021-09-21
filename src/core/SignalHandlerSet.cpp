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
        auto v = sets_[sig - 1];
        for (auto ptr : v)
        {
            ptr->run();
        }
    }
}

SignalHandlerId SignalHandlerSet::add(char signal, SignalCallback cb)
{
    SignalHandler *handler = new SignalHandler(signal, std::move(cb));
    loop_->runInLoop(std::bind(&SignalHandlerSet::addInLoop, this, handler));
    return SignalHandlerId(handler, handler->sequence());
}

void SignalHandlerSet::remove(SignalHandlerId id)
{
    loop_->runInLoop(std::bind(&SignalHandlerSet::removeInLoop, this, id));
}

void SignalHandlerSet::addInLoop(SignalHandler *handler)
{
    loop_->assertInLoopThread();
    sets_[handler->signal() - 1].insert(handler);
    SignalUtils::handleSignal(handler->signal(), SignalUtils::Piped);
}

void SignalHandlerSet::removeInLoop(SignalHandlerId id)
{
    loop_->assertInLoopThread();
    CHECK_NOTNULL(id.signalEvent_);
    auto it = sets_[id.signalEvent_->signal()-1].find(id.signalEvent_);
    if(it != sets_[id.signalEvent_->signal()-1].end())
    {
        sets_[id.signalEvent_->signal()-1].erase(it);
    }
    else{
        LOG_WARN<<"There is not this signal handler";
    }
    if(!sets_[id.signalEvent_->signal()-1].size())
    {
        SignalUtils::handleSignal(id.signalEvent_->signal(), SignalUtils::Default);
    LOG_INFO<<"Debug";
    }
}