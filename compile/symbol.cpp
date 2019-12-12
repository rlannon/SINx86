/*

SIN Toolchain (x86 target)
symbol.cpp

The implementation of the symbol class

*/

#include "symbol.h"

SymbolType symbol::get_symbol_type() const {
    return this->symbol_type;
}

symbol::symbol(std::string name, std::string scope_name, unsigned int scope_level, DataType type_information, unsigned int stack_offset) : 
    name(name),
    scope_name(scope_name),
    scope_level(scope_level),
    type(type_information),
    stack_offset(stack_offset)
{
    this->symbol_type = VARIABLE;
}

symbol::symbol() {
    this->symbol_type = VARIABLE;
}

symbol::~symbol() {
    // todo: destructor
}
