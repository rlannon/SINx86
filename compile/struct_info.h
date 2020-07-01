#pragma once

/*

SIN Toolchain (x86 target)
struct_info.h
Copyright 2020 Riley Lannon

Definition of a class to contain information about struct definitions

*/

#include <unordered_map>
#include <vector>
#include <string>

#include "symbol.h"
#include "compile_util/symbol_table.h"
#include "../util/Exceptions.h"

class struct_info {
    std::string struct_name;
    symbol_table members;    // struct members are proper symbols within the struct's scope

    bool width_known;   // will be false if the struct wasn't defined
    size_t struct_width;
public:
    std::string get_struct_name() const;   // get the struct's name
    symbol& get_member(std::string name);    // get the member with a given name
    size_t get_width() const;   // get the struct's width (0 if unknown)
    bool is_width_known() const;    // whether the width of the struct is known

    struct_info(std::string struct_name, std::vector<symbol> members, unsigned int line);
    struct_info(std::string struct_name);
    struct_info();
    ~struct_info();
};
