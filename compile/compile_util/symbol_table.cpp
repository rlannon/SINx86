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

	std::unordered_map<std::string, std::shared_ptr<symbol>>::iterator it = this->symbols.find(
		//symbol_table::get_mangled_name(to_erase.name)
		to_erase.name
	);
	if (it == this->symbols.end()) {
		throw std::exception();
	}
	else {
		this->symbols.erase(it);
	}
}

std::string symbol_table::get_mangled_name(std::string org) {
	/*

	get_mangled_name
	Gets the mangled symbol name

	SIN adds 'SIN_' to all symbol names

	*/

	return "SIN_" + org;
}

bool symbol_table::insert(std::shared_ptr<symbol> to_insert) {
	/*
	
	insert
	Inserts a symbol into the table
	
	Note that since a shared pointer is used, any type inheriting from symbol may be used provided it is casted appropriately when retrieved
	
	*/

	// we have to make sure the symbol table doesn't include copies of data with names unmangled
	if (this->contains(to_insert->get_name())) {
		return false;
	}

	auto returned = this->symbols.insert(
		std::make_pair<>(
			//symbol_table::get_mangled_name(to_insert->get_name()),
			to_insert->get_name(),
			to_insert)
	);	// should std::unordered_map::emplace be used instead of insert?

	if (returned.second) {
		this->locals.push_back(
			node(
				to_insert->get_name(),
				to_insert->get_scope_name(),
				to_insert->get_scope_level()
			)
		);
	}
	else {
		// todo: specialize exceptions thrown here
		throw std::exception();
	}

	return returned.second;
}

bool symbol_table::contains(std::string symbol_name)
{
	// returns whether the symbol with a given name is in the symbol table
	// if it can't find it with the name mangled, it will try finding the unmangled version
	bool in_table = false;	
	if ((bool)this->symbols.count(
		symbol_table::get_mangled_name(symbol_name)
	)) {
		in_table = true;
	}
	else {
		in_table = (bool)this->symbols.count(symbol_name);
	}
	return in_table;
}

std::shared_ptr<symbol>& symbol_table::find(std::string to_find)
{
	/*
	
	find
	Returns an iterator to the desired symbol

	If it can't find the symbol with the name mangled, it tries to find the unmangled version
	
	*/

	auto it = this->symbols.find(
		symbol_table::get_mangled_name(to_find)
	);
	if (it == this->symbols.end()) {
		it = this->symbols.find(to_find);

		if (it == this->symbols.end())
			throw std::exception();
	}

	return it->second;
}

std::vector<symbol> symbol_table::get_symbols_to_free(std::string name, unsigned int level, bool is_function) {
	/*

	get_symbols_to_free
	Gets a list of symbols that need to have their references decremented before the scope is left
	
	Gets local variables that satisfy the following conditions:
		* is marked as dynamic
		* is a pointer
		* is a reference
	If we are in a function, we need to

	*/

	std::vector<symbol> v;
	stack<node> l = this->locals;
	while (
		!l.empty() && 
		(is_function ? 
			(l.peek().scope_level <= level) :
			(l.peek().scope_level == level)	&& 
		l.peek().scope_name == name)
	) {
		symbol s = *this->find(l.pop_back().name);
		if (
			s.get_data_type().get_primary() == PTR ||
			s.get_data_type().get_qualities().is_dynamic()
		) {
			v.push_back(s);
		}
	}

	return v;
}

void symbol_table::leave_scope(std::string name, unsigned int level)
{
	/*
	
	leave_scope
	Leaves the current scope, deleting all variables local to that scope

	Any data that should be freed by the GC should be returned in a vector
	
	*/
	
	// so that exceptions aren't thrown, always make sure our stack isn't empty before we look at it
	if (!this->locals.empty()) {
		// create a sentinel variable
		bool done = false;

		while (!this->locals.empty() && this->locals.peek().scope_level == level && this->locals.peek().scope_name == name) {
			// pop the last node and erase it
			node to_erase = this->locals.pop_back();

			// ensure that we don't delete symbols from the global scope
			if (to_erase.scope_name != "global") {
				this->erase(to_erase);
			}
		}
	}
}

std::vector<std::shared_ptr<symbol>> symbol_table::get_all_symbols() {
	// gets all symbols
	std::vector<std::shared_ptr<symbol>> v;
	for (auto s: this->symbols) {
		v.push_back(s.second);
	}

	return v;
}

symbol_table::symbol_table()
{
}

symbol_table::~symbol_table()
{
}
