/*

Stack.h
An implementation of a template data structure

Currently, this implementation utilizes std::vector at its core. Once the newer version (that utilizes std::allocator) is properly debugged, this will be changed back. However, it was causing significant memory access bugs due to improper reallocation procedures.

*/

#pragma once

#include <vector>
#include <exception>
#include <stdexcept>

template <typename T>
class stack
{
	std::vector<T> data;
public:
	void push_back(T const& elem);
	T pop_back();
	T& peek();

	bool empty() const;

	stack();
	~stack();
};

template <typename T>
inline void stack<T>::push_back(T const& elem)
{
	this->data.push_back(elem);
	return;
}

template <typename T>
inline T stack<T>::pop_back()
{
	if (this->data.size() == 0)
	{
		throw std::out_of_range("Stack<T>: cannot pop from empty stack");
	}
	else {
		T to_return = this->data.back();
		this->data.pop_back();
		return to_return;
	}
}

template <typename T>
inline T& stack<T>::peek()
{
	if (this->data.size() == 0)
	{
		throw std::out_of_range("Stack<T>: cannot peek empty stack");
	}
	else
	{
		return data[data.size() - 1];
	}
}

template <typename T>
inline bool stack<T>::empty() const
{
	return this->data.empty();
}

template <typename T>
inline stack<T>::stack()
{
}

template <typename T>
inline stack<T>::~stack()
{
}
