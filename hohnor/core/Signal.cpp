#include "Signal.h"
#include "EventLoop.h"

using namespace Hohnor;

void SignalHandler::createIOHandler(int fd, std::function<void()> cb)
{
    if(cb == nullptr){
        LOG_WARN << "Creating signal io without callback";
    }
    HCHECK(fd > 0)<< "fd is not created properly";
    int signal = signal_;
    ioHandler_ = loop_->handleIO(fd);
    ioHandler_->setReadCallback([fd, signal, cb](){
        SignalUtils::signalFDRead(fd, signal);
        cb();
    });
    ioHandler_->enable();
}

SignalHandler::SignalHandler(EventLoop* loop, int signal, SignalUtils::SigAction action, std::function<void()> cb): loop_(loop), signal_(signal), action_(action), ioHandler_(nullptr)
{
    int fd = SignalUtils::handleSignal(signal, action);
    if(action == SignalUtils::SigAction::Handled){
        createIOHandler(fd, cb);
    }
}

void SignalHandler::update(SignalUtils::SigAction action, std::function<void()> cb)
{
    int oldAction = action_;
    action_ = action;
    if(oldAction == action && action == SignalUtils::Handled) //Switching callback in handle
    {
        ioHandler_->setReadCallback(cb);
        return;
    }
    else if(oldAction == SignalUtils::Handled && action != oldAction){ //Does not need IOHandler now, switch to default or ignore
        ioHandler_->disable();
        ioHandler_ = nullptr;
        SignalUtils::handleSignal(signal_, action);
    }
    else{
        int fd = SignalUtils::handleSignal(signal_, action);
        if(action == SignalUtils::SigAction::Handled){ //switched to handle
            createIOHandler(fd, cb);
        }
    }
}