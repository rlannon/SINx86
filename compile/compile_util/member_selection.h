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
public:
	static member_selection create_member_selection(Binary& exp, struct_table& structs, symbol_table& symbols, unsigned int line);

	void append(symbol& to_add);

	symbol& last();
	symbol& first();

	member_selection operator=(member_selection& right);

	member_selection();
	~member_selection();
};
