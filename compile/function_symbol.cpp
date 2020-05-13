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
	call_con(call_con)
{
    /*

    specialized constructor

    This constructor will construct an object for the function signature and determine which arguments can be passed in which registers

    */

    if (this->formal_parameters.size() > 0) {
        // this->arg_regs will hold the registers used by this signature

        size_t stack_offset = 0;  // the current stack offset 

        // todo: this routine will be different for each of our 

        // act based on calling convention
		if (call_con == calling_convention::SINCALL) {
			// determine the register for each of our formal parameters
			bool can_pass_in_reg = true;	// once we have one argument passed on the stack, all subsequent arguments will be
			for (symbol &sym : this->formal_parameters) {
				// which register is used (or whether a register is used at all) depends on the primary type of the symbol
				Type primary_type = sym.get_data_type().get_primary();

				if (can_pass_in_reg && (primary_type != ARRAY && primary_type != STRUCT && primary_type != STRING)) {
					// pass in the primary type; the get_available_register function will be able to handle it
					reg to_use = NO_REGISTER;
					const reg integer_registers[] = { RSI, RDI, RCX, RDX, R8, R9 };
					const reg float_registes[] = { XMM0, XMM1, XMM2, XMM3, XMM4, XMM5 };

					bool found = false;
					if (primary_type == FLOAT) {
						unsigned short i = 0;
						while (i < 6 && !found) {
							if (this->arg_regs.is_in_use(float_registes[i])) {
								i++;
							}
							else {
								found = true;
							}
						}

						if (found)
							to_use = float_registes[i];
					}
					else {
						unsigned short i = 0;
						while (i < 6 && !found) {
							if (this->arg_regs.is_in_use(integer_registers[i]))
								i++;
							else
								found = true;
						}

						if (found)
							to_use = integer_registers[i];
					}

					// set the symbol's register
					sym.set_register(to_use);

					// if the argument must go on the stack, so must all subsequent arguments
					if (to_use == NO_REGISTER) {
						can_pass_in_reg = false;
					}
					else {
						this->arg_regs.set(to_use);	// mark the register as in use
					}
				}
				else {
					// string, array, and struct types use pointers, though it's possible arrays and structs will be constructed on the stack (instead of always in dynamic memory) as their widths might be known at compile time

					// todo: string, array, struct argument passing
					can_pass_in_reg = false;	// once one argument is passed on the stack, all subsequent arguments must be as well
				}
			}
		}
		else {
			throw CompilerException("Currently, no other calling conventions are supported", compiler_errors::INVALID_SYMBOL_TYPE_ERROR, 0);
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
