#include "hohnor/io/FdUtils.h"
#include <fcntl.h>
#include <dirent.h>
#include <string>
#include <iostream>
#include <atomic>
#include <termios.h>
#include "hohnor/log/Logging.h"

namespace Hohnor
{
    namespace FdUtils
    {
    } // namespace FdUtils
} // namespace Hohnor

using namespace Hohnor;
using namespace Hohnor::FdUtils;

int Hohnor::FdUtils::setNonBlocking(int fd, bool nonBlocking)
{
    int oldOptions = fcntl(fd, F_GETFL);
    if (oldOptions == -1)
    {
        LOG_SYSERR << "FdUtils::setNonBlocking error";
        return -1;
    }
    int newOptions = nonBlocking ? (oldOptions | O_NONBLOCK) : (oldOptions & ~(O_NONBLOCK));
    int retVal = fcntl(fd, F_SETFL, newOptions);
    if (retVal == -1)
    {
        LOG_SYSERR << "FdUtils::setNonBlocking error";
        return -1;
    }
    return oldOptions;
}

int FdUtils::setCloseOnExec(int fd, bool closeOnExec)
{
    int oldOptions = fcntl(fd, F_GETFD);
    if (oldOptions == -1)
    {
        LOG_SYSERR << "FdUtils::setCloseOnExec error (F_GETFD)";
        return -1;
    }

    int newOptions = closeOnExec ? (oldOptions | FD_CLOEXEC)
                                 : (oldOptions & ~FD_CLOEXEC);

    int retVal = fcntl(fd, F_SETFD, newOptions);
    if (retVal == -1)
    {
        LOG_SYSERR << "FdUtils::setCloseOnExec error (F_SETFD)";
        return -1;
    }

    return oldOptions;
}


void FdUtils::close(int fd)
{
    if (::close(fd) < 0)
    {
        LOG_SYSERR << "close " << fd << " error";
    }
}

static bool is_fd_in_procfs(int fd) {
    std::string path = "/proc/self/fd/" + std::to_string(fd);
    return access(path.c_str(), F_OK) == 0;
}

void FdGuard::setFd(int fd)
{
    HCHECK(is_fd_in_procfs(fd)) << "The fd trying to guard is not open to the process yet";
    fd_ = fd;
}

// Store original terminal attributes to restore them on exit
static struct termios saved_attributes;
static std::atomic<bool> terminal_interactive_mode(false);

// Function to reset terminal to its original settings
void FdUtils::resetInputInteractive(void) {
    if(terminal_interactive_mode.load() == false) {
        LOG_WARN << "Terminal is not in interactive mode.";
        return;
    }
    terminal_interactive_mode.store(false);
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
    printf("\nTerminal mode restored.\n");
}

// Function to set terminal to non-canonical, no-echo mode
void FdUtils::setInputInteractive(void) {
    if(terminal_interactive_mode.load()) {
        LOG_WARN << "Terminal is already in interactive mode.";
        return;
    }
    terminal_interactive_mode.store(true);
    struct termios tattr;

    // Make sure stdin is a terminal
    HCHECK(isatty(STDIN_FILENO)) << "Not a terminal.";

    // Save the original terminal attributes
    tcgetattr(STDIN_FILENO, &saved_attributes);
    // Register the reset function to be called on exit
    atexit(resetInputInteractive);

    // Get the current attributes
    tcgetattr(STDIN_FILENO, &tattr);
    // Disable canonical mode and echo
    tattr.c_lflag &= ~(ICANON | ECHO);
    // Set the terminal to read one character at a time
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    // Apply the new attributes
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &tattr);
    LOG_INFO << "Terminal iteractive mode.";
}