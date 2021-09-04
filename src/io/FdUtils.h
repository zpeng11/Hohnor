/**
 * File descriptor utillities
 */
#pragma once
#include <unistd.h>
#include <memory>
#include "NonCopyable.h"

namespace Hohnor
{
    namespace FdUtils
    {
        /**
         * class that guards the file descriptor, use RAII to release resource.
         * Please Allows use this with std::shared_ptr.
         */
        class FdGuard : NonCopyable
        {
        private:
            int fd_;

        public:
            FdGuard(int fd) : fd_(fd) {}
            int fd() { return fd_; }
            ~FdGuard() { close(fd_); }
        };

    } // namespace FdUtils

} // namespace Hohnor

// /**
//          * Zero copy IOs
//          */
//         //Direct use for sendfile(2)
//         constexpr auto sendfile = ::sendfile;
//         //Direct use for splice(2)
//         constexpr auto splice = ::splice;
//         //Direct use for tee(2), simular to sendfile(2) but does not consume data from src
//         constexpr auto tee = ::tee;