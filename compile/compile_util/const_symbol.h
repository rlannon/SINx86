#pragma once

/*

SIN Toolchain (x86 target)
const_symbol.h
Copyright 2020 Riley Lannon

The compile-time expression evaluator needs to store the symbol data, as well as the initial values, for constants

*/

#include "../symbol.h"
#include "../../parser/Expression.h"

class const_symbol : public symbol {
	// track the constant's value with a string; because we know the type, we can cast appropriately
	std::string value;
public:
	std::string get_value();

	const_symbol(symbol s, std::string v);
	~const_symbol();
};
