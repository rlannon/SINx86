/*

SIN Toolchain (x86 target)
register_usage.h
Copyright 2019 Riley Lannon

A class to track the current general-purpose registers in use

*/


#include <unordered_map>
#include "../../util/EnumeratedTypes.h"
#include "../../util/Exceptions.h"


class register_usage {
    /*

    Uses booleans to track which registers are currently in use

    */

    // using an unordered_map will allow much easier access to these booleans
    std::unordered_map<reg, bool&> regs;

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

    register_usage();
    ~register_usage();
};