/*

SIN Toolchain (x86 target)
symbol.cpp

The implementation of the symbol class

*/

#include "symbol.h"

SymbolType symbol::get_symbol_type() const {
	// get the symbol's type
    return this->symbol_type;
}

reg symbol::get_register() const {
	// get the register that currently contains the symbol data
    return this->current_reg;
}

void symbol::set_register(reg to_set) {
	// change register where the symbol currently resides
    this->current_reg = to_set;
}

std::string symbol::get_name() const {
	// get the symbol name
	return this->name;
}

std::string symbol::get_scope_name() const {
	// get the name of the symbol's scope
	return this->scope_name;
}

unsigned int symbol::get_scope_level() const
{
	// get the scope level of the symbol
	return this->scope_level;
}

DataType symbol::get_data_type() const {
	// get the DataType object of the symbol
	return this->type;
}

unsigned int symbol::get_stack_offset() const {
	// get the symbol's offset from the stack frame base
	return this->stack_offset;
}

bool symbol::was_initialized() const {
	// check whether the symbol was initialized
    return this->initialized;
}

bool symbol::was_freed() const {
	// check whether the symbol was freed
    return this->freed;
}

void symbol::set_initialized() {
	// mark the symbol as initialized
    this->initialized = true;
}

void symbol::free() {
	// mark a symbol as having been freed
    this->freed = true;
}

symbol::symbol(std::string name, std::string scope_name, unsigned int scope_level, DataType type_information, unsigned int stack_offset) : 
    name(name),
    scope_name(scope_name),
    scope_level(scope_level),
    type(type_information),
    stack_offset(stack_offset)
{
    this->current_reg = NO_REGISTER;
    this->symbol_type = VARIABLE;
    this->freed = false;    // symbols should start as allocated
	this->initialized = false;
}

symbol::symbol(): symbol("", "", 0, DataType(), 0) {
	// delegating constructor
}

symbol::~symbol() {
    // todo: destructor
}
