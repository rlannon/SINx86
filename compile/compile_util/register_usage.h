/*

SIN Toolchain (x86 target)
register_usage.h
Copyright 2019 Riley Lannon

A class to track the current general-purpose registers in use

*/

#pragma once

#include <unordered_map>
#include <vector>
#include <utility>
#include <string>

#include "../../util/EnumeratedTypes.h"
#include "../../util/Exceptions.h"
#include "../../util/DataType.h"
#include "../symbol.h"

// forward declaration to allow this to be used by 'store_all_symbols'
std::stringstream store_symbol(const symbol& s);

class register_usage {
    /*

    Uses booleans to track which registers are currently in use

    */

    struct node {
        bool in_use;
        bool has_been_used;
        symbol* contained;

        node();
        ~node();
    };

    // todo: track WHICH symbol is being held by that register

    // using an unordered_map will allow much easier access to these booleans
    std::unordered_map<reg, node> regs;  // first = currently in use; second = has been used
    static std::unordered_map<reg, std::string> reg_strings;
    static std::unordered_map<reg, std::string> reg_32_strings;
    static std::unordered_map<reg, std::string> reg_16_strings;
    static std::unordered_map<reg, std::string> reg_8_strings;
    
    // track if a register has been used at all
    bool used[24] = { false };

    // register test functions
    static bool is_int_register(reg to_test);
    static bool is_xmm_register(reg to_test);
public:
    static const std::vector<reg> all_regs;

    bool is_in_use(reg to_test) const;    // whether the register is _currently_ in use
    bool was_used(reg to_test) const; // whether the register was used at all

    symbol* get_contained_symbol(reg r); // checks whether the register contains a symbol
    void clear_contained_symbol(reg r); // sets the contained symbol to nullptr

    std::string store_all_symbols();   // stores all symbols currently in registers into their respective memory locations

    // todo: change to one function, 'set_available' ?
    void set(reg to_set, symbol* s=nullptr);
    void clear(reg to_clear);

    // for getting the first available register
    reg get_available_register(Type data_type);

    // to determine whether we can pass an argument in that register
    static bool is_valid_argument_register(reg to_check, calling_convention call_con);
    
    // get the name of a register
    static std::string get_register_name(const reg to_get);    // full 64-bit register
    static std::string get_register_name(const reg to_get, const DataType& t); // get the appropriate name based on type width

    register_usage();
    ~register_usage();
};