#include "hohnor/core/Signal.h"
#include "hohnor/core/EventLoop.h"
#include "hohnor/log/Logging.h"
#include "hohnor/core/IOHandler.h"
#include <signal.h>
#include <sys/signalfd.h>


using namespace Hohnor;

int handleSignal(int signal, SignalAction action){
    if (signal <= 0 || signal >= 65)
        LOG_FATAL << "Invalid signal value";

    // Set up the signal mask Back to default for all cases
    struct sigaction sa;
    memZero(&sa, sizeof sa);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signal);
    
    if (action == SignalAction::Handled){
        // Set signal action to default to prevent ignore
        sa.sa_handler = SIG_DFL;
        sa.sa_flags = 0;
        if (::sigaction(signal, &sa, NULL) < 0)
            LOG_SYSERR << "sigaction error";
        // Block signal so it doesn't get handled by the default handler
        if (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1)
            LOG_FATAL << "sigprocmask error";

        // Create signalfd for signal
        int sfd = signalfd(-1, &mask, SFD_CLOEXEC);
        if (sfd == -1) 
            LOG_FATAL << "signalfd error";
        return sfd;
    }

    // Unblock signal so it is handled again by default handler
    if (sigprocmask(SIG_UNBLOCK, &mask, nullptr) == -1)
        LOG_FATAL << "sigprocmask error";
    // Set the signal action to default or ignore
    if (action == SignalAction::Ignored)
        sa.sa_handler = SIG_IGN;
    else
        sa.sa_handler = SIG_DFL;
    sa.sa_flags |= SA_RESTART;
    if (::sigaction(signal, &sa, NULL) < 0)
        LOG_SYSERR << "sigaction error";
    return -1;
}

void signalFDRead(int fd, int signal){
    struct signalfd_siginfo fdsi;
    ssize_t s = read(fd, &fdsi, sizeof(fdsi));
    HCHECK_EQ(s, sizeof(fdsi)) << "Read signal fd size error";
    HCHECK_EQ(fdsi.ssi_signo, signal) << "Received unexpected signal: " << fdsi.ssi_signo;
}

void SignalHandler::createIOHandler(int fd, Functor cb)
{
    if(cb == nullptr){
        LOG_WARN << "Creating signal io without callback";
    }
    HCHECK(fd > 0)<< "fd is not created properly";
    int signal = signal_;
    ioHandler_ = loop_->handleIO(fd);
    ioHandler_->setReadCallback([fd, signal, cb](){
        signalFDRead(fd, signal);
        cb();
    });
    ioHandler_->enable();
}

SignalHandler::SignalHandler(std::shared_ptr<EventLoop> loop, int signal, SignalAction action, SignalCallback cb): loop_(loop), signal_(signal), action_(action), ioHandler_(nullptr)
{
    int fd = handleSignal(signal, action);
    if(action == SignalAction::Handled){
        createIOHandler(fd, cb);
    }
}

void SignalHandler::update(SignalAction action, SignalCallback cb)
{
    int oldAction = action_;
    action_ = action;
    if(oldAction == action && action == SignalAction::Handled) //Stay in handle but change callback
    {
        ioHandler_->setReadCallback(cb);
        return;
    }
    else if(oldAction == SignalAction::Handled && action != oldAction){ //Switch from handle to default or ignore
        ioHandler_->disable();
        handleSignal(signal_, action);
    }
    else{//switch from default or ignore
        if(action == SignalAction::Handled){ //switched to handle
            if(ioHandler_){
                LOG_DEBUG << "SignalHandler already has an IOHandler, updating callback";
                ioHandler_->setReadCallback(cb);
                ioHandler_->enable();
            }
            else{
                LOG_DEBUG << "Creating new IOHandler for signal " << signal_;
                int fd = handleSignal(signal_, action);
                createIOHandler(fd, cb);
            }
        }
        else{ //switch to ignore or default
            handleSignal(signal_, action);
        }
    }
}

void SignalHandler::disable()
{
    handleSignal(signal_, SignalAction::Default);
}

