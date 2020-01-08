/*

SIN Toolchain (x86 target)
functions.cpp
Copyright 2020 Riley Lannon

This file contains all of the functionality to define and call functions

*/

#include "compiler.h"

std::stringstream compiler::define_function(FunctionDefinition definition) {
    /*

    define_function
    Generates the code for a function definition

    All functions assume the SIN calling convention, similar to C calling conventions.
    The procedure for function code generation is essentially:
        - push a new register_usage object to the compiler's stack
        - the current compiler scope is updated
        - control is transfered to generate code for the definition's procedure
        - the register stack is analyzed to determine which registers are used by the function; code is then generated to preserve these registers (and restore them)

    @param  definition  The definition statement for which code is being generated
    @return A stringstream containing the generated code

    */

    std::stringstream definition_ss;    // contains the code for the entire callee
    std::stringstream procedure_ss; // contains the code for the actual procedure

    // todo: push a new register_usage object to our stack

    // todo: compile function using compiler::compile_ast, passing to it as a parameter the definition's procedure

    // todo: determine registers used by the function and generate code to preserve and restore them

    // todo: put all of the generated code together in definition_ss

    return definition_ss;
}

std::stringstream compiler::call_function(Call call) {
    /*

    call_function
    Generates the code to call a function

    Performs all of the necessary functions for the caller in the SIN calling convention
    This includes setting up the call stack, calling the function, and performing any necessary compiler object updates
    Note that this function delegates to the appropriate code generation function based on what the calling convention is
    
    For more information on compiler calling conventions, see doc/Calling Convention.md

    @param  call    The call statement to compile
    @return A stringstream containing the generated code

    */

    std::stringstream call_ss;

    // first, look up the function
    std::shared_ptr<symbol> sym = this->lookup(call.get_func_name(), call.get_line_number());
    if (sym->get_symbol_type() == FUNCTION_SYMBOL) {
        // cast to the correct type
        function_symbol func_sym = *dynamic_cast<function_symbol*>(sym.get());

        // behaves according to the calling convention
        if (func_sym.get_calling_convention() == SINCALL) {
            // SIN calling convention
            call_ss << this->sincall(func_sym, call.get_args(), call.get_line_number()).str(); // todo: return this function's result directly?
        } else {
            // todo: other calling conventions
        }

        // now, the function's return value (or pointer to the return value) is in RAX
    } else {
        throw InvalidSymbolException(call.get_line_number());
    }

    // todo: any miscellaneous clean-up?
    return call_ss;
}

std::stringstream compiler::sincall(function_symbol s, std::vector<std::shared_ptr<Expression>> args, unsigned int line) {
    /*

    sincall
    Generates stack set-up code for the SIN calling convention

    For more information on this calling convention, see doc/Calling Convention.md

    @param  s   The symbol for the function
    @param  args    The function's arguments
    @param  line    The line number where the call occurs
    @return A stringstream containing the generated code

    */

    std::stringstream sincall_ss;

    // iterate over our arguments, ensure the types match and that we have an appropriate number
    for (std::shared_ptr<Expression> sp: args) {
        // todo: type checking
        // todo: expression evaluation
        // todo: track which registers have been used, determine whether we can pass in registers or whether we must pass on the stack
    }

    // todo: preserve registers in use
    // todo: pass/push arguments
    // todo: call function
    // todo: restore used registers

    return sincall_ss;
}
