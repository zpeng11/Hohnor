/**
 * Handling signal to prevent interupt, send them to a socket pipe and allow epoll to handle
 */
//TODO: Enable onFork()
#pragma once
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <functional>
#include <vector>
#include "FdUtils.h"
#include "Logging.h"

namespace Hohnor
{
    namespace SignalUtils
    {

        /**
         * Iterate recieving signal results.
         */
        class Iter : Copyable
        {
        private:
            const char *sig_;
            ssize_t position_;
            ssize_t size_;

        public:
            explicit Iter(const char *signals, ssize_t size) : sig_(signals), position_(0), size_(size) {}
            ~Iter() = default;
            inline bool hasNext() { return position_ < size_; }
            inline ssize_t size() { return size_; }
            //Return signal number 
            inline char next()
            {
                return sig_[position_++];
            }
        };

        enum SigAction
        {
            Ignored,
            Default,
            Piped //put signal into pipe for epoll to receive
        };
        //Handle a signal with callback function, thread safe
        void handleSignal(int signal, SigAction action = Piped);

        //Receive signals and use a iterator to acess them
        Iter receive();

        int readEndFd();
    }

} // namespace Hohnor
