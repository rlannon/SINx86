/*

SIN Toolchain (x86 target)
const_symbol.cpp
Copyright 2020 Riley Lannon

Implementation of the const_symbol class

*/

#include "const_symbol.h"

// todo: class implementation

std::string const_symbol::get_value()
{
	// Returns the symbol's const value
	return this->value;
}

const_symbol::const_symbol(symbol s, std::string v) :
	symbol(s.get_name(), s.get_scope_name(), s.get_scope_level(), s.get_data_type(), s.get_offset()), value(v)
{
	// super called
}

const_symbol::const_symbol() :
	symbol(), value(std::string())
{
	// super called
}

const_symbol::~const_symbol()
{
}
