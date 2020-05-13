#pragma once

/*

SIN Toolchain (x86 target)
member_selection.h
Copyright 2020 Riley Lannon

A class to contain information about dot/arrow expressions so that they may be properly evaluated

*/

#include <list>

#include "../symbol.h"
#include "symbol_table.h"
#include "struct_table.h"
#include "../../parser/Expression.h"

class member_selection {
	std::list<symbol> symbols;
	std::list<symbol>::iterator it;	// todo: devise better way to iterate through elements?

	// this should only be accessed by this class
	void append(symbol& to_add);
public:
	static member_selection create_member_selection(Binary& exp, struct_table& structs, symbol_table& symbols, unsigned int line);

	// get a reference to a member
	symbol& last();
	symbol& first();

	// uses the internal iterator
	symbol& peek();
	symbol& next();
	symbol& peek_previous();
	symbol& previous();
	bool is_at_end();

	member_selection& operator=(member_selection right);

	member_selection(Binary &exp, struct_table &structs, symbol_table &symbols, unsigned int line);
	member_selection();
	~member_selection();
};
