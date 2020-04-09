#pragma once

/*

SIN Toolchain (x86 target)
member_selection.h
Copyright 2020 Riley Lannon

A class to contain information about dot/arrow expressions so that they may be properly evaluated

*/

#include <list>

#include "../symbol.h"

class member_selection {
	std::list<symbol&> symbols;
public:
	void append(symbol& to_add);

	symbol& last() const;
	symbol& first() const;

	member_selection();
	~member_selection();
};
