#pragma once
#include <memory>
#include "Types.h"
#include "NonCopyable.h"

namespace Hohnor
{
	/**
	 * T type requires to have default constructor, assignment operator
	 */
	template <typename T>
	class CircularBuffer : Hohnor::NonCopyable
	{
	private:
		//Use c array T[] because vector is too dense in this case, wrap it with share_ptr so that we do not take care of its life cycle
		std::unique_ptr<T[]> buffer;
		std::size_t head = 0;
		std::size_t tail = 0;
		std::size_t max_size;
		T empty_item;

	public:
		explicit CircularBuffer(std::size_t maxSize) : buffer(), max_size(maxSize){
			buffer = std::move(std::unique_ptr<T[]>(new T[max_size]));
			if(maxSize<2)
			{
				throw new std::runtime_error("Size is not allowed");
			}
		}
		// Add an item to this circular buffer.
		void enqueue(const T &item)
		{
			// if buffer is full, throw an error
			if (full())
				throw std::runtime_error("buffer is full");
			// insert item at back of buffer
			buffer[tail] = item;
			// increment tail
			tail = (tail + 1) % max_size;
		}

		void enqueue(T &&item)
		{
			// if buffer is full, throw an error
			if (full())
				throw std::runtime_error("buffer is full");
			// insert item at back of buffer
			buffer[tail] = std::move(item);
			// increment tail
			tail = (tail + 1) % max_size;
		}

		// Remove an item from this circular buffer and return it.
		T dequeue()
		{

			// if buffer is empty, throw an error
			if (empty())
				throw std::runtime_error("buffer is empty");

			// get item at head
			T item = T(std::move(buffer[head]));

			// set item at head to be empty
			T empty_item;
			buffer[head] = empty_item;

			// move head foward
			head = (head + 1) % max_size;

			// return item
			return item;
		}

		// Return the item at the front of this circular buffer.
		T front() { return buffer[head]; }

		// Return true if this circular buffer is empty, and false otherwise.
		bool empty() { return head == tail; }

		// Return true if this circular buffer is full, and false otherwise.
		bool full() { 
			return (tail + 1) % max_size == head; }

		// Return the size of this circular buffer.
		size_t size() const
		{
			if (tail >= head)
				return tail - head;
			return max_size - head - tail;
		}

		size_t capacity() const
		{
			return max_size;
		}
	};
}
