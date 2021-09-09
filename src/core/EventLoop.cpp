#include "EventLoop.h"
#include "Timestamp.h"


using namespace Hohnor;


void EventLoop::loop()
{
    while(!quit_)
    {
        Timestamp now = Timestamp::now();
        now.microSecondsSinceEpoch();
        //update epoll wait time according to timer event

        //epoll Wait for any events

        //Get Timer ,timer event run

        //Get functors out from events and put into pendingFunctors

        //Run pendingFunctors
    }
}