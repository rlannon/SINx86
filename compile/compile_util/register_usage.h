/*

SIN Toolchain (x86 target)
register_usage.h
Copyright 2019 Riley Lannon

A class to track the current general-purpose registers in use

*/

#pragma once

#include <unordered_map>
#include <string>

#include "../../util/EnumeratedTypes.h"
#include "../../util/Exceptions.h"


class register_usage {
    /*

    Uses booleans to track which registers are currently in use

    */

    // using an unordered_map will allow much easier access to these booleans
    std::unordered_map<reg, bool&> regs;
    static const std::unordered_map<reg, std::string> reg_strings;
    static const std::unordered_map<reg, std::string> reg_32_strings;
    static const std::unordered_map<reg, std::string> reg_16_strings;

    // The four general-purpose named registers
    bool rax;
    bool rbx;
    bool rcx;
    bool rdx;

    // source and data registers
    bool rsi;
    bool rdi;

    // additional registers for x86_64
    // Data widths are 64 = r8; 32 = r8d; 16 = r8w; 8 = r8b
    bool r8;
    bool r9;
    bool r10;
    bool r11;
    bool r12;
    bool r13;
    bool r14;
    bool r15;
public:
    bool is_in_use(reg to_test);
    void set(reg to_set);

    // for getting the first available register
    reg get_available_register();
    
    // get the name of a register
    std::string get_register_name(reg to_get) const;    // full 64-bit register
    std::string get_r32_name(reg to_get) const; // 32-bit name
    std::string get_r16_name(reg to_get) const; // 16-bit name

    register_usage();
    ~register_usage();
};