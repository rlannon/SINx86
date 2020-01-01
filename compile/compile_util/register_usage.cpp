/*

SIN Toolchain (x86 target)
register_usage.cpp
Copyright 2019 Riley Lannon

The implementation of the register_usage class

*/

#include "register_usage.h"

std::unordered_map<reg, std::string> register_usage::reg_strings {
    { RAX, "rax" },
    { RBX, "rbx" },
    { RCX, "rcx" },
    { RDX, "rdx" },
    { RSI, "rsi" },
    { RDI, "rdi" },
    { R8, "r8" },
    { R9, "r9" },
    { R10, "r10" },
    { R11, "r11" },
    { R12, "r12" },
    { R13, "r13" },
    { R14, "r14" },
    { R15, "r15" }
};

std::unordered_map<reg, std::string> register_usage::reg_32_strings {
    { RAX, "eax" },
    { RBX, "ebx" },
    { RCX, "ecx" },
    { RDX, "edx" },
    { RSI, "esi" },
    { RDI, "edi" },
    { R8, "r8d" },
    { R9, "r9d" },
    { R10, "r10d" },
    { R11, "r11d" },
    { R12, "r12d" },
    { R13, "r13d" },
    { R14, "r14d" },
    { R15, "r15d" }
};

std::unordered_map<reg, std::string> register_usage::reg_16_strings {
    { RAX, "ax" },
    { RBX, "bx" },
    { RCX, "cx" },
    { RDX, "dx" },
    { R8, "r8w" },
    { R9, "r9w" },
    { R10, "r10w" },
    { R11, "r11w" },
    { R12, "r12w" },
    { R13, "r13w" },
    { R14, "r14w" },
    { R15, "r15w" }
};

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
        throw CompilerException("Invalid register selection");
    } else {
        it->second = true;  // it is a reference, so it will update the original
    }
}

void register_usage::clear(reg to_clear) {
    // Marks a register as available
    std::unordered_map<reg, bool&>::iterator it = this->regs.find(to_clear);
    if (it == this->regs.end()) {
        throw CompilerException("Invalid register selection");
    } else {
        it->second = false; // since it's a reference, it will update the original
    }
}

reg register_usage::get_available_register() {
    // Returns the first available register

    // iterate through the registers until we find one that isn't in use
    std::unordered_map<reg, bool&>::iterator it = this->regs.begin();
    while (it != this->regs.end() && it->second) {
        it++;
    }

    if (it == this->regs.end()) {
        throw CompilerException("No registers available");
    } else {
        return it->first;
    }
}

std::string register_usage::get_register_name(const reg to_get) {
    // Get the string value of a register name
	std::unordered_map<reg, std::string>::iterator it = reg_strings.find(to_get);
	if (it == reg_strings.end()) {
		// todo: exception here?
		throw CompilerException("Invalid register selection");
	} else {
		return it->second;
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
