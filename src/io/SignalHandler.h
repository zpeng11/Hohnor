/**
 * Handling signal to prevent interupt, send them to a socket pipe and allow epoll to handle
 */

#pragma once
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "FdUtils.h"
#include "Logging.h"

namespace Hohnor
{
    class SignalHandler : NonCopyable
    {
    private:
        int pipefd_[2];
        //buffer for reading pipe
        char signals_[512];
        //return value of reading pipe
        int ret_;
        explicit SignalHandler();
        ~SignalHandler();
        class Iter
        {
        private:
            SignalHandler *ptr_;
            int position_;

        public:
            Iter(SignalHandler *ptr) : ptr_(ptr), position_(0) {}
            bool hasNext() { return position_ < ptr_->ret_; }
            ssize_t size() { return ptr_->ret_; }
            char next()
            {
                CHECK(position_ < ptr_->ret_) << " Hohnor::SignalHandler::Iter::next() error";
                return ptr_->signals_[position_++];
            }
            ~Iter() { ptr_->ret_ = 0; }
        };

    public:
        static SignalHandler &getInst();
        void addSig(int signal);
        Iter receive();
        int fd() const { return pipefd_[0]; }
    };

} // namespace Hohnor
