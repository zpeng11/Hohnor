#pragma once

namespace Hohnor
{
	/**
	 * NonCopyable class, any class who inheret this would not be able to use copy constructor and equal operator 
	 */
	class NonCopyable
	{
	public:
		NonCopyable(const NonCopyable &) = delete;
		void operator=(const NonCopyable &) = delete;

	protected:
		NonCopyable() = default;
		~NonCopyable() = default;
	};
}