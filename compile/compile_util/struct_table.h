#pragma once

/*

SIN Toolchain (x86 target)
struct_table.h
Copyright 2020 Riley Lannon

Contains the definition of the struct_table class. The purpose of the class is to implement a table that contains information about all user-defined structs.

*/

#include <unordered_map>
#include <string>

#include "../struct_info.h"
#include "../symbol.h"

class struct_table {
	std::unordered_map<std::string, struct_info> structs;
public:
	bool insert(struct_info to_add);
	bool contains(std::string name);
	struct_info& find(const std::string& name, const unsigned int line);
	const struct_info& find(const std::string& name, const unsigned int line) const;

	struct_table();
	~struct_table();
};
