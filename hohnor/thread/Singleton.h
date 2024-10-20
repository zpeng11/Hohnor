/**
 * Thread singleton that has only one instance in the whole process.
 * Reminder the template type should have default constructor.
 */

#pragma once

#include "common/NonCopyable.h"
#include <assert.h>
#include <pthread.h>
#include <stdlib.h> // atexit

namespace Hohnor
{

	// This doesn't detect inherited member functions!
	// http://stackoverflow.com/questions/1966362/sfinae-to-check-for-inherited-member-functions
	template <typename T>
	struct has_no_destroy
	{
		template <typename C>
		static char test(decltype(&C::no_destroy));
		template <typename C>
		static int32_t test(...);
		const static bool value = sizeof(test<T>(0)) == 1;
	};

	template <typename T>
	class Singleton : NonCopyable
	{
	public:
		Singleton() = delete;
		~Singleton() = delete;

		static T &instance()
		{
			pthread_once(&ponce_, &Singleton::init);
			assert(value_ != NULL);
			return *value_;
		}

	private:
		static void init()
		{
			value_ = new T();
			if (!has_no_destroy<T>::value)
			{
				::atexit(destroy);
			}
		}

		static void destroy()
		{
			typedef char T_must_be_complete_type[sizeof(T) == 0 ? -1 : 1];
			T_must_be_complete_type dummy;
			(void)dummy;

			delete value_;
			value_ = NULL;
		}

	private:
		static pthread_once_t ponce_;
		static T *value_;
	};

	template <typename T>
	pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

	template <typename T>
	T *Singleton<T>::value_ = NULL;

} // namespace muduo