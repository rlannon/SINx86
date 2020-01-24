/*

SIN Toolchain (x86 target)
functions.cpp
Copyright 2020 Riley Lannon

This file contains all of the functionality to define and call functions

*/

#include "compiler.h"

void compiler::handle_declaration(Declaration decl_stmt) {
    /*

    declare_function
    Adds a function's signature to the symbol table

    This function generates no code; it simply adds a symbol/signature to the symbol table so that it may be utilized properly by the program.
    Note that the caller is responsible for ensuring the symbol is in the global scope at level 0

    @param  decl_stmt   The declaration from which we are generating a symbol
    @throws Nothing directly; called functions may throw exceptions

    */

    if (decl_stmt.is_function()) {
        function_symbol sym = create_function_symbol(decl_stmt);
        this->add_symbol(sym, decl_stmt.get_line_number());
    } else if (decl_stmt.is_struct()) {
        // todo: add struct to struct table with the caveat that it's an incomplete type
    } else {
        // add a symbol
        symbol sym = generate_symbol(decl_stmt, this->current_scope_name, this->current_scope_level, this->max_offset);
        this->add_symbol(sym, decl_stmt.get_line_number());
    }
}

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

    // save current scope info
    std::string previous_scope_name = this->current_scope_name;
    unsigned int previous_scope_level = this->current_scope_level;
    size_t previous_max_offset = this->max_offset;

    // todo: should be package this together in an object and use a stack to keep track of it? that might make things a little cleaner

    // update the scope info -- name = function name, level = 1, offset = 0
    this->current_scope_name = definition.get_name();
    this->current_scope_level = 1;
    this->max_offset = 0;

    // construct the symbol for the function -- everything is offloaded to the utility
    function_symbol func_sym = create_function_symbol(definition);

    // add the symbol to the table
    this->add_symbol(func_sym, definition.get_line_number());

    // now, we have to iterate over the function symbol's parameters and add them to our symbol table
    // todo: optimize by enabling symbol table additions in template function?
    std::set<reg> arg_regs;
    for (symbol &sym: func_sym.get_formal_parameters()) {
        // add the symbol to the table and update our stack offset
        this->add_symbol(func_sym, definition.get_line_number());
        reg r = sym.get_register();
        if (r != NO_REGISTER) {
            arg_regs.insert(sym.get_register());
        }
    }

    // update the stack offset -- since symbols are pushed in order, just get the last one; if we had no parameters, the offset should be 0
    if (func_sym.get_formal_parameters().size() != 0) {
        const symbol &last_sym = func_sym.get_formal_parameters().back(); 
        this->max_offset = last_sym.get_data_type().get_width() + last_sym.get_stack_offset();
    } else {
        this->max_offset = 0;
    }

    // get the register_usage object from func_sym and push that
    this->reg_stack.push_back(func_sym.get_arg_regs());

    // if the function is 'main', then we must define the program's entry point as this function
    if (func_sym.get_name() == "main") {
        definition_ss << "global _start" << std::endl;
        definition_ss << "_start:" << std::endl;
    }

    // add a label for the function
    definition_ss << func_sym.get_name() << ":" << std::endl;

    // now, compile the procedure using compiler::compile_ast, passing in this function's signature
    procedure_ss = this->compile_ast(*definition.get_procedure().get(), std::make_shared<function_symbol>(func_sym));

    // our register saving and clean-up will be affected by the calling convention
    if (definition.get_calling_convention() == SINCALL) {
        // todo: any clean up that the callee has to do

        // originally, this convention was going to preserve and restore registers used by the function, but this screws with our stack offsets, so unless we push and restore all registers before each call, we will not preserve registers
    } else {
        throw CompilerException("Currently, calling convention specification is not supported", compiler_errors::ILLEGAL_QUALITY_ERROR, definition.get_line_number());
    }

    // now, put everything together in definition_ss by adding procedure_ss onto the end
    definition_ss << procedure_ss.str() << std::endl;

    // our register stack was already popped; don't do it again

    // todo: clean up our symbol table (delete local variables from it)? or is iterating through and deleting these variables not worth it?

    // restore our scope information
    this->current_scope_name = previous_scope_name;
    this->current_scope_level = previous_scope_level;
    this->max_offset = previous_max_offset;

    return definition_ss;
}

std::stringstream compiler::call_function(Call call) {
    /*

    call_function
    Generates the code to call a function

    Performs all of the necessary functions for the caller in the SIN calling convention
    This includes setting up the call stack, calling the function, and performing any necessary compiler object updates
    Note that this function delegates to the appropriate code generation function based on what the calling convention is

    Note that the return value will always be moved into either RAX or XMM0, depending on its type, whatever the calling convention is
    
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
            throw CompilerException("Other calling conventions not supported at this time", 0, call.get_line_number());
        }

        // now, the function's return value (or pointer to the return value) is in RAX or XMM0, depending on the type -- the compiler always expects return values here regardless of calling convention
        // further, all clean-up has been handled, so we can continue code generation
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
    We don't need to touch max_offset here when making the pushes and pops to and from the stack because the stack pointer will be returned to normal when the function returns

    @param  s   The symbol for the function
    @param  args    The function's arguments
    @param  line    The line number where the call occurs
    @return A stringstream containing the generated code

    */

    std::stringstream register_preservation_ss; // for preserving registers used for argument passing
    std::stringstream sincall_ss;

    // get the formal parameters so we don't need to call a function every time
    std::vector<symbol> &formal_parameters = s.get_formal_parameters();

    // ensure the number of arguments provided is less than or equal to the number expected
    if (args.size() <= s.get_formal_parameters().size()) {
        // now, preserve our stack frame
        sincall_ss << "\t" << "pushq rbp" << std::endl;
        sincall_ss << "\t" << "mov rbp, rsp" << std::endl;

        // iterate over our arguments, ensure the types match and that we have an appropriate number
        std::vector<symbol>::iterator it = formal_parameters.begin();
        for (std::shared_ptr<Expression> arg: args) {
            // first, ensure the types match
            DataType arg_type = get_expression_data_type(arg, this->symbol_table, line);
            if (arg_type.is_compatible(it->get_data_type())) {
                // evaluate the expression and pass it in the appropriate manner
                sincall_ss << this->evaluate_expression(arg, line).str();

                // now, determine where that data should go -- this has been determined already so we don't need to do it on every function call
                // todo: pass value
            } else {
                // if the types don't match, we have a signature mismatch
                throw FunctionSignatureException(line);
            }

            // increment our parameter iterator; since args.size is <= formal_parameters.size, we don't have to worry about it running past the end of the vector
            it++;
        }

        // next, call the function
        sincall_ss << "\t" << "call " << s.get_name() << std::endl;

        // the return value is now in RAX or XMM0, depending on the data type

        // now, restore the old stack frame
        sincall_ss << "\t" << "mov rsp, rbp" << std::endl;
        sincall_ss << "\t" << "popq rbp" << std::endl;
    } else {
        // If the number of arguments supplied exceeds the number expected, throw an error -- the call does not match the signature
        throw FunctionSignatureException(line);
    }

    // finally, return our call code
    return sincall_ss;
}

std::stringstream compiler::handle_return(ReturnStatement ret, function_symbol signature) {
    /*

    handle_return
    Generates code for a return statement

    This function generates a return statement, ensuring the return type matches the signature's expected return value. Further, if the function name is 'main', then we will generate a call to sys_exit, passing the return value in RBX

    @param  ret The return statement for which we are generating code
    @param  signature   The function signature in which the statement occurs

    @return The generated code

    @throws This function throws an exception if the return values do not match the function signature

    */

    std::stringstream ret_ss;

    // first, ensure that the return statement's data type is compatible with the signature
    if (get_expression_data_type(ret.get_return_exp(), this->symbol_table, ret.get_line_number()).is_compatible(signature.get_data_type())) {
        // types are compatible; how the value gets returned (and how the callee gets cleaned up) depends on the function's calling convention
        if (signature.get_calling_convention() == SINCALL) {
            ret_ss << this->sincall_return(ret).str() << std::endl;
        } else {
            // todo: other calling conventions; for now, throw an exception
            throw CompilerException("Calling conventions other than sincall are currently not supported", 0, ret.get_line_number());
        }
    } else {
        throw ReturnMismatchException(ret.get_line_number());
    }

    return ret_ss;
}

std::stringstream compiler::sincall_return(ReturnStatement &ret) {
    /*

    sincall_return
    Handles a return statement for a function using the sincall calling convention

    This function is _not_ responsible for restoring used registers; it simply evaluates the expression and returns it according to the calling convention. This means that if an object is returned on the stack, the *callee* (this function) is responsible for allocating memory for it.

    */

    std::stringstream sincall_ss;

    // how the value is returned depends on the data type
    DataType return_type = get_expression_data_type(ret.get_return_exp(), this->symbol_table, ret.get_line_number());

    // all we need to do for integral and floating point types is evaluate the expression; the result will be in either RAX or XMM0, respectively; other types require a little bit more work
    if (return_type == ARRAY) {
        // todo: array evaluation
    } else if (return_type == STRUCT) {
        // todo: struct evaluation
    } else if (return_type == STRING) {
        // todo: string evaluation
    } else if (return_type == FLOAT) {
        // evaluate the expression
        sincall_ss << evaluate_expression(ret.get_return_exp(), ret.get_line_number()).str() << std::endl;
    } else {
        sincall_ss << evaluate_expression(ret.get_return_exp(), ret.get_line_number()).str() << std::endl;
    }

    return sincall_ss;
}
