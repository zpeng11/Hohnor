#include "NonCopyable.h"
#include <stdint.h>
namespace Hohnor
{
	//Just for study CAS, we will use std::atomic
#ifdef USE_MY_ATOMIC
	namespace Atomic
	{
		template <class T>
		class AtomicInt : NonCopyable
		{
		private:
			volatile T value; //visable for all threads
		public:
			AtomicInt() : value(0) {}

			AtomicInt(const AtomicInt &that) : value(that.get()) {}
			AtomicInt &operator=(const AtomicInt &rhs)
			{
				this->getAndSet(rhs.get());
				return *this;
			}
			T get()
			{
				//CAS(addr, old, new) equals
				//int a = *addr; if(a == old){*addr = new;} return a;
				//that three sentence syn at machine level.
				//Here we set old and new be the same,
				//that means asking the program to attend a CAS race and return value previewsly on address;
				//sounds simular to directly returning value on address,
				//but we informed the machine we are asking CAS lock
				//the machine can optimize for it.
				return __sync_val_compare_and_swap(&value, 0, 0);
			}
			T getAndSet(T newVal)
			{
				return __sync_lock_test_and_set(&value, newValue);
			}
			T getAndAdd(T x)
			{
				return __sync_fetch_and_add(&value, x);
			}

			T addAndGet(T x)
			{
				return getAndAdd(x) + x;
			}

			T incrementAndGet()
			{
				return addAndGet(1);
			}

			T decrementAndGet()
			{
				return addAndGet(-1);
			}

			void add(T x)
			{
				getAndAdd(x);
			}

			void increment()
			{
				incrementAndGet();
			}

			void decrement()
			{
				decrementAndGet();
			}
		};
	}
#endif //use my atomic
}