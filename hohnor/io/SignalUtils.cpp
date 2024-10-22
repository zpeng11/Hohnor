#include "SignalUtils.h"
#include "hohnor/log/Logging.h"
#include "hohnor/time/Timestamp.h"
#include <sys/socket.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <set>

namespace Hohnor
{
    namespace Handler
    {

        int g_pipefd[2];

        const static int retSize = 512;
        char g_retSignals[retSize];

        void sigHandle(int sig)
        {
            int save_errno = errno;
            char msg = static_cast<char>(sig);
            if (send(g_pipefd[1], (char *)&msg, 1, 0) <= 0)
                LOG_SYSERR << "Handling signal and putting message into pipe error";
            errno = save_errno;
        }
        void afterFork()
        {
            if (socketpair(AF_UNIX, SOCK_STREAM, 0, g_pipefd) < 0)
                LOG_SYSERR << "At fork socketpair creation error";

            //Nonblocking write so that writing to pipe would not be blocked(signal handle should return fast!)
            FdUtils::setNonBlocking(g_pipefd[1]);
            //close on exec so that different process doesnot share same pipe(processes should have own SignalHandler)
            FdUtils::setCloseOnExec(g_pipefd[0]);
            FdUtils::setCloseOnExec(g_pipefd[1]);
            memZero(g_retSignals, 512);
        }
        class SignalHandlerInitilizer
        {
        public:
            SignalHandlerInitilizer()
            {
                afterFork();
                pthread_atfork(NULL, NULL, afterFork);
                //by default ignore SIGPIPE 
                SignalUtils::handleSignal(SIGPIPE, SignalUtils::Ignored);
            }
            ~SignalHandlerInitilizer()
            {
                FdUtils::close(g_pipefd[0]);
                FdUtils::close(g_pipefd[1]);
            }
        };
        SignalHandlerInitilizer init;
    } // namespace handle

} // namespace Hohnor

using namespace Hohnor;
using namespace Hohnor::Handler;

SignalUtils::Iter SignalUtils::receive()
{
    int ret = ::recv(g_pipefd[0], g_retSignals, retSize, 0);
    if (ret == -1)
    {
        LOG_SYSERR << "SignalUtils::receive() Handler recieve error";
    }
    else if (ret == 0 && errno != EINTR)
    {
        LOG_ERROR << "SignalUtils::receive() Handler recieve error, writing end may be closed";
    }
    return Iter(g_retSignals, ret);
}

int SignalUtils::createSignalFD(int sig){
    int sfd;
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, sig);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1){//Block default signal action
        LOG_SYSERR << "sigprocmask block failed";
    }
    return signalfd(-1, &mask, 0);
}

void SignalUtils::handleSignal(int sig, SigAction action)
{
    if (sig <= 0 || sig >= 65)
        LOG_FATAL << "Invalid signal value";
    struct sigaction sa;
    memZero(&sa, sizeof sa);
    if (action == Piped)
        sa.sa_handler = sigHandle;
    if (action == Ignored)
        signal(sig, SIG_IGN);
    else
        sa.sa_handler = SIG_DFL;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);//To protect safe when setting up
    //::sigaction(3) is signal safe, so we do not need to surround it with signal blocking guard
    if (::sigaction(sig, &sa, NULL) < 0)
        LOG_SYSERR << "SignalUtils action settle failed";
}

int SignalUtils::readEndFd()
{
    return g_pipefd[0];
}