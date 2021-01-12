/*

SIN Toolchain (x86 target)
register_usage.cpp
Copyright 2019 Riley Lannon

The implementation of the register_usage class

*/

#include "register_usage.h"

register_usage::node::node() {
    this->in_use = false;
    this->has_been_used = false;
    this->contained = nullptr;
}

register_usage::node::~node() {
    // nothing to do here
}

const std::vector<reg> register_usage::all_regs {
    RAX,
    RBX,
    RSI,
    RDI,
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,
    XMM0,
    XMM1,
    XMM2,
    XMM3,
    XMM4,
    XMM5,
    XMM6,
    XMM7
};

std::unordered_map<reg, std::string> register_usage::reg_strings {
    { RAX, "rax" },
    { RBX, "rbx" },
    { RCX, "rcx" },
    { RDX, "rdx" },
    { RSI, "rsi" },
    { RDI, "rdi" },
    { R8, "r8" },   // todo: there is a better way to do this
    { R9, "r9" },
    { R10, "r10" },
    { R11, "r11" },
    { R12, "r12" },
    { R13, "r13" },
    { R14, "r14" },
    { R15, "r15" },
    { XMM0, "xmm0" },   // todo: there is a better way to do this
    { XMM1, "xmm1" },
    { XMM2, "xmm2" },
    { XMM3, "xmm3" },
    { XMM4, "xmm4" },
    { XMM5, "xmm5" },
    { XMM6, "xmm6" },
    { XMM7, "xmm7" }
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
    { R15, "r15d" },
    { XMM0, "xmm0" },   // todo: there is a better way to do this
    { XMM1, "xmm1" },
    { XMM2, "xmm2" },
    { XMM3, "xmm3" },
    { XMM4, "xmm4" },
    { XMM5, "xmm5" },
    { XMM6, "xmm6" },
    { XMM7, "xmm7" }
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

std::unordered_map<reg, std::string> register_usage::reg_8_strings {
    { RAX, "al" },
    { RBX, "bl" },
    { RCX, "cl" },
    { RDX, "dl" },
    { RSI, "sil" },
    { RDI, "dil" },
    { R8, "r8b" },
    { R9, "r9b" },
    { R10, "r10b" },
    { R11, "r11b" },
    { R12, "r12b" },
    { R13, "r13b" },
    { R14, "r14b" },
    { R15, "r15b" }
};

bool register_usage::is_int_register(reg to_test) {
    // Returns whether the register is one of our 64-bit integer registers
    return !is_xmm_register(to_test);
}

bool register_usage::is_xmm_register(reg to_test) {
    // Returns whether the register is one of the 128-bit xmm registers
    return (
        to_test == XMM0 ||
        to_test == XMM1 ||
        to_test == XMM2 ||
        to_test == XMM3 ||
        to_test == XMM4 ||
        to_test == XMM5 ||
        to_test == XMM6 ||
        to_test == XMM7
    );
}

bool register_usage::is_in_use(reg to_test) const {
    // Returns whether the specified register is in use
    auto it = this->regs.find(to_test);
    bool b = false;
    // in practice, this error should never occur -- but check anyway to be safe
    if (it == this->regs.end()) {
        throw CompilerException("Invalid register choice");
    } else {
        b = it->second.in_use;
    }
    return b;
}

bool register_usage::was_used(reg to_test) const {
    // Returns whether the specified register has been used at all
    auto it = this->regs.find(to_test);
    bool b = false;
    if (it == this->regs.end()) {
        throw CompilerException("Invalid register choice");
    } else {
        b = it->second.has_been_used;
    }
    return b;
}

symbol* register_usage::get_contained_symbol(reg r) {
    // Returns the symbol in the specified register
    auto it = this->regs.find(r);
    symbol *s = nullptr;
    if (it == this->regs.end()) {
        throw CompilerException("Invalid register choice");
    }
    else {
        s = it->second.contained;
    }
    return s;
}

void register_usage::clear_contained_symbol(reg r) {
    // Does not clear the register from being 'in use', but sets the symbol to nullptr
    auto it = this->regs.find(r);
    if (it == this->regs.end()) {
        throw CompilerException("Invalid reigster choice");
    }
    else {
        it->second.contained = nullptr;
    }
}

void register_usage::set(reg to_set, symbol* s) {
    // Sets a given register to 'in use'
    auto it = this->regs.find(to_set);
    
    // in practice, this error should never occur -- but check anyway to be safe
    if (it == this->regs.end()) {
        throw CompilerException("Invalid register selection");
    } else {
        it->second.in_use = true;  // it is a reference, so it will update the original
        it->second.has_been_used = true;   // mark the register as having been used
        it->second.contained = s;   // update the symbol we are pointing to
        if (s != nullptr) {
            s->set_register(to_set);    // make sure the symbol says it's using this register, too
        }
    }
}

void register_usage::clear(reg to_clear) {
    // Marks a register as available
    auto it = this->regs.find(to_clear);
    if (it == this->regs.end()) {
        throw CompilerException("Invalid register selection");
    } else {
        it->second.in_use = false; // since it's a reference, it will update the original
        it->second.contained = nullptr;
    }
}

reg register_usage::get_available_register(Type data_type) {
    // Returns the first available register for the desired type

    // use a function pointer to enable us to test whether the register is for its or floats based on the data type we pass in
    bool (*is_type)(reg);
    if (data_type == FLOAT) {
        is_type = is_xmm_register;
    } else if (data_type != ARRAY && data_type != STRUCT && data_type != STRING) {
        is_type = is_int_register;
    } else {
        return NO_REGISTER;
    }

    // iterate through the registers until we find one that isn't in use and is of the type we want
    // call is_type via implicit dereferencing
    auto it = this->regs.begin();
    while (it != this->regs.end() && (it->second.in_use || !is_type(it->first)) ) {
        it++;
    }

    // return NO_REGISTER if none are available; else, return the register
    if (it == this->regs.end()) {
        return NO_REGISTER;
    } else {
        return it->first;
    }
}

bool register_usage::is_valid_argument_register(const reg to_check, const calling_convention call_con) {
    /*

    is_valid_argument_register
    Checks validity of a register for argument passing
    
    A utility function that determines whether we can pass an argument in the specified register in the specified calling convention.

    @param  to_check    The register about which we are inquiring
    @param  call_con    The calling convention to use

    @return Whether the specified register can be used to pass an argument in the calling convention

    */

    bool is_valid = false;

    // first, if we have NO_REGISTER, we obviously can't pass it via a register
    if (to_check == NO_REGISTER) {
        is_valid = false;
    } else {
        // which registers are available depends on the calling convention
        if (call_con == SINCALL) {
            // todo: this can definitely be improved for readability, maintainability, etc.

            if (to_check == XMM0 || to_check == XMM1 || to_check == XMM2 || to_check == XMM3 || to_check == XMM4 || to_check == XMM5) {
                is_valid = true;
            } else if (to_check == RSI || to_check == RDI || to_check == RCX || to_check == RDX || to_check == R8 || to_check == R9) {
                is_valid = true;
            } else {
                is_valid = false;
            }
        } else {
            throw CompilerException("Currently, SINCALL is the only available calling convention", 0, 0);
        }
    }

    return is_valid;
}

std::string register_usage::get_register_name(const reg to_get) {
    // Get the string value of a register name
	auto it = reg_strings.find(to_get);
	if (it == reg_strings.end()) {
		// todo: is the exception here necessary?
		throw CompilerException("Invalid register selection");
	} else {
		return it->second;
	}
}

std::string register_usage::get_register_name(const reg to_get, DataType t) {
    // Get the string value of a register name based on its width
    std::unordered_map<reg, std::string>::const_iterator it;
    bool found = false;
    if (t.get_width() == 4) {
        it = reg_32_strings.find(to_get);
        found = it != reg_32_strings.end();
    } else if (t.get_width() == 2) {
        it = reg_16_strings.find(to_get);
        found = it != reg_16_strings.end();
    } else if (t.get_width() == 1) {
        it = reg_8_strings.find(to_get);
        found = it != reg_8_strings.end();
    } else {
        it = reg_strings.find(to_get);
        found = it != reg_strings.end();
    }

    if (!found) {
        throw CompilerException("Invalid register selection");
    }

    return it->second;
}

register_usage::register_usage(): 
    regs({
		{RAX, node()},
		{RBX, node()},
		{RCX, node()},
		{RDX, node()},
		{RSI, node()},
		{RDI, node()},
		{R8, node()},
		{R9, node()},
		{R10, node()},
		{R11, node()},
		{R12, node()},
		{R13, node()},
		{R14, node()},
		{R15, node()},
		{XMM0, node()},
		{XMM1, node()},
		{XMM2, node()},
		{XMM3, node()},
		{XMM4, node()},
		{XMM5, node()},
		{XMM6, node()},
		{XMM7, node()}
	})
{
    // nothing to do here
}

register_usage::~register_usage() {
    // todo: destructor
}
