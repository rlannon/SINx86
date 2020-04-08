/*

SIN Toolchain (x86 target)
symbol_table.cpp
Copyright 2020 Riley Lannon

Implementation of the symbol_table class

*/

#include "symbol_table.h"

/*

symbol_table::node

*/

symbol_table::node::node(std::string name, std::string scope_name, unsigned int scope_level):
	name(name),
	scope_name(scope_name),
	scope_level(scope_level)
{

}

symbol_table::node::node() {

}

symbol_table::node::~node() {

}

/*

symbol_table

*/

void symbol_table::erase(node to_erase) {
	/*
	
	erase
	Erases a symbol from the table by using a node object
	
	Note this does nothing to the stack, as the stack is used to determine which symbols need to be erased

	*/

	std::unordered_map<std::string, std::shared_ptr<symbol>>::iterator it = this->symbols.find(to_erase.name);
	if (it == this->symbols.end()) {
		throw std::exception();
	}
	else {
		this->symbols.erase(it);
	}
}

bool symbol_table::insert(std::shared_ptr<symbol> to_insert) {
	/*
	
	insert
	Inserts a symbol into the table
	
	Note that since a shared pointer is used, any type inheriting from symbol may be used provided it is casted appropriately when retrieved
	
	*/

	std::pair<std::unordered_map<std::string, std::shared_ptr<symbol>>::iterator, bool> returned = this->symbols.insert(
		std::make_pair<>(to_insert->get_name(), to_insert)
	);	// should std::unordered_map::emplace be used instead of insert?

	if (returned.second) {
		this->locals.push_back(
			node(
				to_insert->get_name(),
				to_insert->get_scope_name(),
				to_insert->get_scope_level()
			)
		);
		return true;
	}
	else {
		// todo: specialize exceptions thrown here
		throw std::exception();
		return false;
	}
}

bool symbol_table::insert(symbol to_insert)
{
	return this->insert(std::make_shared<symbol>(to_insert));
}

bool symbol_table::contains(std::string symbol_name)
{
	// returns whether the symbol with a given name is in the symbol table	
	return (bool)this->symbols.count(symbol_name);
}

std::shared_ptr<symbol>& symbol_table::find(std::string to_find)
{
	/*
	
	find
	Returns an iterator to the desired symbol
	
	*/

	std::unordered_map<std::string, std::shared_ptr<symbol>>::iterator it = this->symbols.find(to_find);
	if (it == this->symbols.end()) {
		throw std::exception();
	}

	return it->second;
}

void symbol_table::leave_scope()
{
	/*
	
	leave_scope
	Leaves the current scope, deleting all variables local to that scope
	
	*/
	
	// so that exceptions aren't thrown, always make sure our stack isn't empty before we look at it
	if (!this->locals.empty()) {
		// get our sentinel variables
		std::string leaving_scope_name = this->locals.peek().scope_name;
		unsigned int leaving_scope_level = this->locals.peek().scope_level;

		while (!this->locals.empty() && this->locals.peek().scope_level == leaving_scope_level && this->locals.peek().scope_name == leaving_scope_name) {
			// pop the last node and erase it
			node to_erase = this->locals.pop_back();
			this->erase(to_erase);
		}
	}
}

symbol_table::symbol_table()
{
}

symbol_table::~symbol_table()
{
}
