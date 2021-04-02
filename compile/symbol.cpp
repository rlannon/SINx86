/*

SIN Toolchain (x86 target)
symbol.cpp

The implementation of the symbol class

*/

#include "symbol.h"

bool symbol::operator==(const symbol& right) const {
	/*

	operator==
	Checks whether two symbols match
	
	This means that:
		- their names are the same
		- their types are the same
		- their scopes are the same

	*/

	return (
		this->name == right.name &&
		this->type == right.type &&
		this->scope_name == right.scope_name &&
		this->scope_level == right.scope_level
	);
}

bool symbol::operator!=(const symbol& right) const {
	return !this->operator==(right);
}

SymbolType symbol::get_symbol_type() const {
	// get the symbol's type
    return this->symbol_type;
}

reg symbol::get_register() const {
	// get the register that currently contains the symbol data
    return this->current_reg;
}

void symbol::set_register(const reg to_set) {
	// change register where the symbol currently resides
    this->current_reg = to_set;
}

void symbol::set_as_parameter() {
	// sets the symbol as being a parameter
	this->is_parameter = true;
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

int symbol::get_offset() const {
	// get the symbol's offset from the stack frame base
	return this->offset;
}

void symbol::set_offset(const int new_offset) {
	// sets the new offset
	this->offset = new_offset;
}

bool symbol::is_defined() const {
	return this->defined;
}

void symbol::set_defined() {
	this->defined = true;
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

void symbol::set_line(const unsigned int l) {
	// sets the line number of the symbol
	this->line_defined = l;
}

unsigned int symbol::get_line_defined() const {
	// gets the line number of the symbol
	return this->line_defined;
}

symbol::symbol(
	const std::string& name,
	const std::string& scope_name,
	const unsigned int scope_level,
	const DataType& type_information,
	const unsigned int stack_offset,
	const bool defined,
	const unsigned int line_defined
): 
    name(name),
    scope_name(scope_name),
    scope_level(scope_level),
    type(type_information),
    offset(stack_offset),
	defined(defined),
	line_defined(line_defined)
{
    this->current_reg = NO_REGISTER;
    this->symbol_type = VARIABLE;
    this->freed = false;    // symbols should start as allocated
    this->initialized = (type.get_primary() == STRUCT); // struct types are always constructed to a default state
	this->is_parameter = false;
}

symbol::symbol(): symbol("", "", 0, DataType(), 0) {
	// delegating constructor
}

symbol::~symbol() {
    // todo: destructor
}
