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


class register_usage {
    /*

    Uses booleans to track which registers are currently in use

    */

    // using an unordered_map will allow much easier access to these booleans
    std::unordered_map<reg, std::pair<bool, bool>> regs;  // first = currently in use; second = has been used
    static std::unordered_map<reg, std::string> reg_strings;
    static std::unordered_map<reg, std::string> reg_32_strings;
    static std::unordered_map<reg, std::string> reg_16_strings;
    
    // track if a register has been used at all
    bool used[24] = { false };

    // register test functions
    static bool is_int_register(reg to_test);
    static bool is_xmm_register(reg to_test);
public:
    static const std::vector<reg> all_regs;

    bool is_in_use(reg to_test) const;    // whether the register is _currently_ in use
    bool was_used(reg to_test) const; // whether the register was used at all

    // todo: change to one function, 'set_available' ?
    void set(reg to_set);
    void clear(reg to_clear);

    // for getting the first available register
    reg get_available_register(Type data_type);

    // to determine whether we can pass an argument in that register
    static bool is_valid_argument_register(reg to_check, calling_convention call_con);
    
    // get the name of a register
    static std::string get_register_name(const reg to_get);    // full 64-bit register
    // std::string get_r32_name(reg to_get); // 32-bit name
    // std::string get_r16_name(reg to_get); // 16-bit name

    register_usage();
    ~register_usage();
};