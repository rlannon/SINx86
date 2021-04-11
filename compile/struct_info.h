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
    symbol *get_member(const std::string& name);    // get the member with a given name
    size_t get_width() const;   // get the struct's width (0 if unknown)
    bool is_width_known() const;    // whether the width of the struct is known

    std::vector<symbol*> get_all_members();
    std::vector<symbol> get_members_to_free();
    std::vector<symbol> &get_members_to_free(std::vector<symbol> &current);

    inline size_t members_size() const noexcept
    {
        return members.num_members();
    }

    struct_info(const std::string& struct_name, const std::vector<std::shared_ptr<symbol>>& members, const unsigned int line);
    struct_info(const std::string& struct_name);
    struct_info(const struct_info &s);
    struct_info();
    ~struct_info();
};
