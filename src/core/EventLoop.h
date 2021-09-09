/**
 * The core of the Hohnor library manages events in loop
 */

#include "NonCopyable.h"
#include <atomic>
namespace Hohnor
{
    class EventLoop :NonCopyable
    {

        private:
        std::atomic<bool> quit_;
    };
} // namespace Hohnor


