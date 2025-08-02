/**
 * Handling signal to prevent interupt, send them to a socket pipe and allow epoll to handle
 */

#pragma once
#include "SignalFDUtils.h"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <sys/signalfd.h>
#include "hohnor/log/Logging.h"

using namespace Hohnor;
using namespace Hohnor::SignalUtils;


int SignalUtils::handleSignal(int signal, SigAction action){
    if (signal <= 0 || signal >= 65)
        LOG_FATAL << "Invalid signal value";

    if (action == Handled){
        // Block signal so it doesn't get handled by the default handler
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, signal);
        if (sigprocmask(SIG_BLOCK, &mask, nullptr) == -1)
            LOG_FATAL << "sigprocmask error";

        // Create signalfd for signal
        int sfd = signalfd(-1, &mask, SFD_CLOEXEC);
        if (sfd == -1) 
            LOG_FATAL << "signalfd error";
        return sfd;
    }

    // Unblock signal so it is handled again by default handler
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signal);
    if (sigprocmask(SIG_UNBLOCK, &mask, nullptr) == -1)
        LOG_FATAL << "sigprocmask error";
    struct sigaction sa;
    memZero(&sa, sizeof sa);
    if (action == Ignored)
        sa.sa_handler = SIG_IGN;
    else
        sa.sa_handler = SIG_DFL;
    sa.sa_flags |= SA_RESTART;
    if (::sigaction(signal, &sa, NULL) < 0)
        LOG_SYSERR << "sigaction error";
    return -1;
}