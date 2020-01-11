/*

SIN Toolchain (x86)
function_symbol.cpp

Implementation of the function_symbol class

*/

#include "function_symbol.h"

calling_convention function_symbol::get_calling_convention() {
    // Get the function's calling convention
    return this->call_con;
}

std::vector<symbol> &function_symbol::get_formal_parameters() {
    // Returns a reference to the function's expected parameters
    return this->formal_parameters;
}

register_usage function_symbol::get_arg_regs() {
    // Returns the registers used by parameters
    return this->arg_regs;
}

// for the time being, at least, all functions must be in the global scope at level 0
function_symbol::function_symbol(std::string function_name, DataType return_type, std::vector<symbol> formal_parameters, calling_convention call_con) :
symbol(
    function_name,
    "global", 
    0, 
    return_type, 
    0
),
formal_parameters(formal_parameters),
call_con(call_con) {
    /*

    specialized constructor

    This constructor will construct an object for the function signature and determine which arguments can be passed in which registers

    */

    if (this->formal_parameters.size() > 0) {
        // this->arg_regs will hold the registers used by this signature

        size_t stack_offset = 0;  // the current stack offset 

        // todo: this routine will be different for each of our 

        // determine the register for each of our formal parameters
        for (symbol &sym: this->formal_parameters) {
            // which register is used (or whether a register is used at all) depends on the primary type of the symbol
            Type primary_type = sym.get_data_type().get_primary();

            if (primary_type != ARRAY && primary_type != STRUCT && primary_type != STRING) {
                // pass in the primary type; the get_available_register function will be able to handle it
                reg to_use = arg_regs.get_available_register(primary_type);

                // if we have a valid register, then set that register as 'in use'
                if (register_usage::is_valid_argument_register(to_use, call_con)) {
                    this->arg_regs.set(to_use); // mark the register as in use
                }

                // set the symbol's register
                sym.set_register(to_use);
            } else {
                // string, array, and struct types use pointers, though it's possible arrays and structs will be constructed on the stack (instead of always in dynamic memory) as their widths might be known at compile time

                // todo: string, array, struct argument passing
            }
        }
    }

    this->symbol_type = FUNCTION_SYMBOL;
    this->call_con = call_con;
}

function_symbol::function_symbol() :
function_symbol("", NONE, {}) {
    // delegate to specialized constructor
}

function_symbol::~function_symbol() {
    // todo: destructor
}
