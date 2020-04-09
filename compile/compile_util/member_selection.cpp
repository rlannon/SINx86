/*

SIN Toolchain (x86 target)
member_selection.cpp
Copyright 2020 Riley Lannon

Implementation of the member_selection class

*/

#include "member_selection.h"

void member_selection::append(symbol & to_add)
{
	this->symbols.push_back(to_add);
}

symbol & member_selection::last() const
{
	return this->symbols.back();
}

symbol & member_selection::first() const
{
	return this->symbols.front();
}

member_selection::member_selection()
{
}

member_selection::~member_selection()
{
}
