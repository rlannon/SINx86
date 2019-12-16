/*

Algorithms and Data Structures
Copyright 2019 Riley Lannon
stack.h

An implementation of the Stack data structures using C++ templates and STL Allocators

*/

#pragma once

#include <initializer_list>
#include <memory>
#include <stdexcept>

template <typename T, typename Allocator = std::allocator<T>>
class stack
{
	Allocator stack_allocator;
	size_t _size;
	size_t _capacity;

	T* buffer;
public:
	void push_back(T to_push);
	T pop_back();
	T peek();

	size_t max_size() const;
	size_t capacity() const;
	size_t size() const;
	bool empty() const;

	explicit stack(std::initializer_list<T> il);
	explicit stack();
	~stack();
};

/*

Getters

*/

template <typename T, typename Allocator>
size_t stack<T, Allocator>::max_size() const
{
	return this->stack_allocator.max_size();
}

template <typename T, typename Allocator>
size_t stack<T, Allocator>::capacity() const
{
	return this->_capacity;
}

template <typename T, typename Allocator>
size_t stack<T, Allocator>::size() const
{
	return this->_size;
}

template <typename T, typename Allocator>
bool stack<T, Allocator>::empty() const
{
	return this->_size == 0;
}

/*

Push / Peek / Pop

*/

template <typename T, typename Allocator>
void stack<T, Allocator>::push_back(T to_push)
{
	if (this->_capacity == 0)
	{
		// if we have nothing on the stack, allocate space for four elements
		this->buffer = this->stack_allocator.allocate(4);
		this->_capacity = 4;
	}
	else if (this->_size == this->_capacity)
	{
		size_t new_capacity = this->_capacity * 1.5;

		// if the capacity is less than 4, multiply it by two because of rounding
		if (new_capacity < 4)
		{
			new_capacity *= 2;
		}

		T* old_buff_ptr = this->buffer;
		this->buffer = this->stack_allocator.allocate(new_capacity);

		if (this->buffer)
		{
			memcpy(this->buffer, old_buff_ptr, this->_size * sizeof(T));
			this->stack_allocator.deallocate(old_buff_ptr, this->_size);

			this->_capacity = new_capacity;
		}
		else
		{
			throw std::runtime_error("Cannot push back");
		}
	}

	std::allocator_traits<Allocator>::construct(this->stack_allocator, &this->buffer[this->_size], to_push);
	this->_size += 1;

	return;
}

template <typename T, typename Allocator>
T stack<T, Allocator>::pop_back()
{
	T to_return;

	if (this->_size == 0)
	{
		throw std::out_of_range("Cannot pop from empty stack");
	}
	else
	{
		to_return = this->buffer[this->_size - 1];
		std::allocator_traits<Allocator>::destroy(this->stack_allocator, &this->buffer[this->_size - 1]);
		this->_size -= 1;
	}

	return to_return;
}

template <typename T, typename Allocator>
T stack<T, Allocator>::peek()
{
	// returns the top element of the Stack without popping it

	if (this->_size != 0)
	{
		return this->buffer[this->size - 1];
	}
	else
	{
		throw std::out_of_range("Cannot peek on an empty stack");
	}
}

/*

Constructor and destructor

*/

template <typename T, typename Allocator>
stack<T, Allocator>::stack(std::initializer_list<T> il)
{
	/*
	
	Allow our stack to be initialized with an initializer-list
	The list will push the elements _in order_ from left to right, so the left-most element will be pushed first

	*/

	// set up the allocator
	this->stack_allocator = Allocator();

	// allocate space for the initialized values
	this->_capacity = ((il.size() * 1.5) < 4) ? (il.size() * 2) : (il.size() * 1.5);
	this->buffer = this->stack_allocator.allocate(this->_capacity);
	this->_size = 0;

	// now, for every element in il, push to the buffer
	for (T elem: il)
	{
		std::allocator_traits<Allocator>::construct(this->stack_allocator, &this->buffer[this->_size], elem);
		this->_size += 1;
	}
}

template <typename T, typename Allocator>
stack<T, Allocator>::stack()
{
	this->stack_allocator = Allocator();
	this->_size = 0;
	this->_capacity = 0;
	this->buffer = nullptr;
}

template <typename T, typename Allocator>
stack<T, Allocator>::~stack()
{
	// if we still have objects in the stack, destroy them before deallocating the stack's memory
	if (this->_size > 0)
	{
		for (size_t i = 0; i < this->_size; i++)
		{
			std::allocator_traits<Allocator>::destroy(this->stack_allocator, &this->buffer[i]);
			this->_size -= 1;
		}
	}

	// deallocate the memory, set the capacity to 0, set the buffer to nullptr
	this->stack_allocator.deallocate(this->buffer, this->_capacity);
	this->_capacity = 0;
	this->buffer = nullptr;
}
