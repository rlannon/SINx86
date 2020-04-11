/*

SIN Toolchain (x86 target)
member_selection.cpp
Copyright 2020 Riley Lannon

Implementation of the member_selection class

*/

#include "member_selection.h"

member_selection member_selection::create_member_selection(Binary &exp, symbol_table& symbols) {
	/*
	
	create_member_selection
	Creates a member_selection based on a binary expression (operator must be dot or arrow)
	
	*/

	return member_selection();
}

void member_selection::append(symbol & to_add)
{
	this->symbols.push_back(to_add);
}

symbol & member_selection::last()
{
	return this->symbols.back();
}

symbol & member_selection::first()
{
	return this->symbols.front();
}

member_selection::member_selection()
{
}

member_selection::~member_selection()
{
}
