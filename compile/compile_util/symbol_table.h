#pragma once

/*

SIN Toolchain (x86 target)
symbol_table.h
Copyright 2020 Riley Lannon

The symbol table for this compiler is to be implemented through a hash table as well as a stack to keep track of all symbols.
The hash table (std::unordered_map) keeps track of all symbols in all scopes, while the stack keeps track of local variables so that we know which ones must be removed upon exiting a given scope

*/

#include <unordered_map>
#include <string>
#include <memory>

#include "../symbol.h"
#include "../function_symbol.h"
#include "const_symbol.h"
#include "../../util/stack.h"

class symbol_table {
	// we need a node class for the stack object
	class node {
	public:
		std::string name;
		std::string scope_name;
		unsigned int scope_level;

		node(std::string name, std::string scope_name, unsigned int scope_level);
		node();
		~node();
	};

	// private data members
	std::unordered_map<std::string, std::shared_ptr<symbol>> symbols;
	stack<node> locals;

	// private member functions
	void erase(node to_erase);
public:
	// public member functions
	static std::string get_mangled_name(std::string org);
	
	bool insert(std::shared_ptr<symbol> to_insert);

	bool contains(std::string symbol_name);
	std::shared_ptr<symbol> & find(std::string to_find);
	
	void leave_scope(std::string name, unsigned int level);

	// constructor, destructor
	symbol_table();
	~symbol_table();
};
