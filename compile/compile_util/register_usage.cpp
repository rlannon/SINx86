/*

SIN Toolchain (x86 target)
register_usage.cpp
Copyright 2019 Riley Lannon

The implementation of the register_usage class

*/

#include "register_usage.h"

bool register_usage::is_in_use(reg to_test) {
    // Returns whether the specified register is in use
    std::unordered_map<reg, bool&>::iterator it = this->regs.find(to_test);

    // in practice, this error should never occur -- but check anyway to be safe
    if (it == this->regs.end()) {
        throw CompilerException("Invalid register choice");
    } else {
        return it->second;
    }
}

void register_usage::set(reg to_set) {
    // Sets a given register to 'in use'
    std::unordered_map<reg, bool&>::iterator it = this->regs.find(to_set);
    
    // in practice, this error should never occur -- but check anyway to be safe
    if (it == this->regs.end()) {
        throw CompilerException("Invalid register choice");
    } else {
        it->second = true;  // it is a reference, so it will update the original
    }
}

register_usage::register_usage(): 
    regs(
            {
                {RAX, this->rax},
                {RBX, this->rbx},
                {RCX, this->rcx},
                {RDX, this->rdx},
                {RSI, this->rsi},
                {RDI, this->rdi},
                {R8, this->r8},
                {R9, this->r9},
                {R10, this->r10},
                {R11, this->r11},
                {R12, this->r12},
                {R13, this->r13},
                {R14, this->r14},
                {R15, this->r15}
            }
    )
{
    // zero out our booleans
    for(std::unordered_map<reg, bool&>::iterator it = this->regs.begin(); it != this->regs.end(); it++) {
        it->second = false;
    }
}

register_usage::~register_usage() {
    // todo: destructor
}
