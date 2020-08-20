/*

SIN Toolchain (x86 target)
struct_table.cpp
Copyright 2020 Riley Lannon

The implementation of the struct_table class

*/

#include "struct_table.h"

bool struct_table::insert(struct_info to_add) {
	/*
	
	insert
	Adds a struct to the table

	@param	to_add	The object containing our struct's data
	@returns	A boolean telling us whether the object was added successfully
	
	*/

	std::pair<std::string, struct_info> to_insert(to_add.get_struct_name(), to_add);
	auto returned = this->structs.insert(to_insert);	 // insert the pair containing our key/value pair, store the result in a pair
	return returned.second;
}

bool struct_table::contains(std::string name) {
	/*
	
	contains
	Checks to see whether the table contains a struct with the given name
	
	*/

	std::unordered_map<std::string, struct_info>::iterator it = this->structs.find(name);
	return (it != this->structs.end());
}

struct_info& struct_table::find(std::string name, unsigned int line) {
	/*
	
	find
	Finds a struct with the given name and returns a reference to its entry in the table
	
	*/

	std::unordered_map<std::string, struct_info>::iterator it = this->structs.find(name);

	if (it == this->structs.end()) {
		throw UndefinedException(line);
	}
	
	return it->second;
}

struct_table::struct_table() {

}

struct_table::~struct_table() {

}
