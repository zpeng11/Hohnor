/**
 * Handling signal to prevent interupt, send them to a socket pipe and allow epoll to handle
 */

#pragma once
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <functional>
#include <map>
#include "FdUtils.h"
#include "Logging.h"

namespace Hohnor
{
    class SignalHandler : NonCopyable
    {
    private:
        friend void sysSigHandle(int sig);
        int pipefd_[2];
        //buffer for reading pipe
        char signals_[512];
        //return value of reading pipe
        ssize_t readySignals_;
        explicit SignalHandler();
        ~SignalHandler();
        class Iter
        {
        private:
            SignalHandler *ptr_;
            ssize_t position_;

        public:
            Iter(SignalHandler *ptr) : ptr_(ptr), position_(0) {}
            bool hasNext() { return position_ < size(); }
            ssize_t size() { return ptr_->readySignals_; }
            char next()
            {
                CHECK(position_ < ptr_->readySignals_) << " Hohnor::SignalHandler::Iter::next() error";
                return ptr_->signals_[position_++];
            }
            ~Iter() { ptr_->readySignals_ = 0; }
        };

    public:
        static SignalHandler &getInst();
        //Handle a signal with callback function, if leave that with null, ignore the signal
        void addSig(int signal, bool ignore = false);
        Iter receive();
        int fd() const { return pipefd_[0]; }
    };

} // namespace Hohnor
