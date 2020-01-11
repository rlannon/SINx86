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

bool register_usage::is_in_use(reg to_test) {
    // Returns whether the specified register is in use
    std::unordered_map<reg, std::pair<bool&, bool&>>::iterator it = this->regs.find(to_test);

    // in practice, this error should never occur -- but check anyway to be safe
    if (it == this->regs.end()) {
        throw CompilerException("Invalid register choice");
    } else {
        return it->second.first;
    }
}

void register_usage::set(reg to_set) {
    // Sets a given register to 'in use'
    std::unordered_map<reg, std::pair<bool&, bool&>>::iterator it = this->regs.find(to_set);
    
    // in practice, this error should never occur -- but check anyway to be safe
    if (it == this->regs.end()) {
        throw CompilerException("Invalid register selection");
    } else {
        it->second.first = true;  // it is a reference, so it will update the original
        it->second.second = true;   // mark the register as having been used
    }
}

void register_usage::clear(reg to_clear) {
    // Marks a register as available
    std::unordered_map<reg, std::pair<bool&, bool&>>::iterator it = this->regs.find(to_clear);
    if (it == this->regs.end()) {
        throw CompilerException("Invalid register selection");
    } else {
        it->second.first = false; // since it's a reference, it will update the original
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
    std::unordered_map<reg, std::pair<bool&, bool&>>::iterator it = this->regs.begin();
    while (it != this->regs.end() && it->second.first && is_type(it->first)) {
        it++;
    }

    // return NO_REGISTER if none are available; else, return the register
    if (it == this->regs.end()) {
        return NO_REGISTER;
    } else {
        return it->first;
    }
}

static bool is_valid_argument_register(const reg to_check, const calling_convention call_con) {
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
	std::unordered_map<reg, std::string>::iterator it = reg_strings.find(to_get);
	if (it == reg_strings.end()) {
		// todo: is the exception here necessary?
		throw CompilerException("Invalid register selection");
	} else {
		return it->second;
	}
}

register_usage::register_usage(): 
    regs(
            {
                {RAX, std::pair<bool&, bool&>(this->rax, this->used[0])},
                {RBX, std::pair<bool&, bool&>(this->rbx, this->used[1])},
                {RCX, std::pair<bool&, bool&>(this->rcx, this->used[2])},
                {RDX, std::pair<bool&, bool&>(this->rdx, this->used[3])},
                {RSI, std::pair<bool&, bool&>(this->rsi, this->used[4])},
                {RDI, std::pair<bool&, bool&>(this->rdi, this->used[5])},
                {R8, std::pair<bool&, bool&>(this->r8, this->used[6])},
                {R9, std::pair<bool&, bool&>(this->r9, this->used[7])},
                {R10, std::pair<bool&, bool&>(this->r10, this->used[8])},
                {R11, std::pair<bool&, bool&>(this->r11, this->used[9])},
                {R12, std::pair<bool&, bool&>(this->r12, this->used[10])},
                {R13, std::pair<bool&, bool&>(this->r13, this->used[11])},
                {R14, std::pair<bool&, bool&>(this->r14, this->used[12])},
                {R15, std::pair<bool&, bool&>(this->r15, this->used[13])},
                {XMM0, std::pair<bool&, bool&>(this->xmm[0], this->used[14])},
                {XMM1, std::pair<bool&, bool&>(this->xmm[1], this->used[15])},
                {XMM2, std::pair<bool&, bool&>(this->xmm[2], this->used[16])},
                {XMM3, std::pair<bool&, bool&>(this->xmm[3], this->used[17])},
                {XMM4, std::pair<bool&, bool&>(this->xmm[4], this->used[18])},
                {XMM5, std::pair<bool&, bool&>(this->xmm[5], this->used[19])},
                {XMM6, std::pair<bool&, bool&>(this->xmm[6], this->used[19])},
                {XMM7, std::pair<bool&, bool&>(this->xmm[7], this->used[20])}
            }
    )
{
    // zero out our booleans
    for(std::unordered_map<reg, std::pair<bool&, bool&>>::iterator it = this->regs.begin(); it != this->regs.end(); it++) {
        it->second.first = false;
    }
}

register_usage::~register_usage() {
    // todo: destructor
}
