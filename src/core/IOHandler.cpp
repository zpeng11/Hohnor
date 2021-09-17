#include "IOHandler.h"

using namespace Hohnor;

IOHandler::IOHandler(EventLoop *loop, int fd):loop_(loop),fd_(fd),events_(0),revents_(0)
{
    loop_->addIOHandler(this);
}

IOHandler::~IOHandler()
{
    disable();
    loop_->removeIOHandler(this);
}