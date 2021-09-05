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

        //Wrapped close(3) api
        void close(int socketfd);

        //Set the nonblocking flag for fd
        int setNonBlocking(int fd, bool nonBlocking = true);
        //Set close on exec function, so that children process can not share this fd with parent
        int setCloseOnExec(int fd, bool closeOnExec = true);

    } // namespace FdUtils

    /**
         * class that guards the file descriptor, use RAII to release resource.
         * Whenever you get a file descriptor, 
         * you can use this class to ensure it closes when run out of current scope
         */
    class FdGuard : NonCopyable
    {
    private:
        int fd_;

    public:
        FdGuard(int fd = -1) : fd_(fd) {}
        int fd() const { return fd_; }
        void setFd(int fd) { fd_ = fd; }
        ~FdGuard() { FdUtils::close(fd_); }
    };
} // namespace Hohnor
