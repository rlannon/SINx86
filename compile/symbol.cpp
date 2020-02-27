/*

SIN Toolchain (x86 target)
symbol.cpp

The implementation of the symbol class

*/

#include "symbol.h"

SymbolType symbol::get_symbol_type() const {
    return this->symbol_type;
}

reg symbol::get_register() const {
    return this->current_reg;
}

void symbol::set_register(reg to_set) {
    this->current_reg = to_set;
}

std::string symbol::get_name() const {
	return this->name;
}

std::string symbol::get_scope_name() const {
	return this->scope_name;
}

unsigned int symbol::get_scope_level() const {
	return this->scope_level;
}

DataType symbol::get_data_type() const {
	return this->type;
}

unsigned int symbol::get_stack_offset() const {
	return this->stack_offset;
}

bool symbol::was_initialized() const {
    return this->initialized;
}

bool symbol::was_freed() const {
    return this->freed;
}

void symbol::set_initialized() {
    this->initialized = true;
}

void symbol::free() {
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

}

symbol::~symbol() {
    // todo: destructor
}
