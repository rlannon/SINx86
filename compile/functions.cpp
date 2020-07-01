/*

SIN Toolchain (x86 target)
functions.cpp
Copyright 2020 Riley Lannon

This file contains all of the functionality to define and call functions

*/

#include "compiler.h"

std::stringstream compiler::handle_declaration(Declaration decl_stmt) {
    /*

    declare_function
    Adds a function's signature to the symbol table

    This function generates no code; it simply adds a symbol/signature to the symbol table so that it may be utilized properly by the program.
    Note that the caller is responsible for ensuring the symbol is in the global scope at level 0

    @param  decl_stmt   The declaration from which we are generating a symbol
    @throws Nothing directly; called functions may throw exceptions

    */

    std::stringstream decl_ss;

    if (decl_stmt.is_function()) {
        // note that declared data must be marked as 'extern' so the assembler can reference it
        function_symbol sym = create_function_symbol(decl_stmt);
        this->add_symbol(sym, decl_stmt.get_line_number());
        decl_ss << "extern " << sym.get_name() << std::endl;
    } else if (decl_stmt.is_struct()) {
        // add struct to struct table with the caveat that it's an incomplete type - this means that member access is not possible
        // note 'extern' is not needed here -- no symbol information is created
        struct_info s_info(decl_stmt.get_type_information().get_struct_name());
        this->add_struct(s_info, decl_stmt.get_line_number());
    } else {
        // add a symbol
        // note: pass 0 as the data width because declared data doesn't occupy stack space
        symbol sym = generate_symbol(decl_stmt, 0, this->current_scope_name, this->current_scope_level, this->max_offset);
        this->add_symbol(sym, decl_stmt.get_line_number());
        decl_ss << "extern " << sym.get_name() << std::endl;
    }

    return decl_ss;
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
	Note that while functions are required to return a value, this check is done elsewhere (see compiler.cpp)

    @param  definition  The definition statement for which code is being generated
    @return A stringstream containing the generated code

    */

    std::stringstream definition_ss;    // contains the code for the entire callee
    std::stringstream procedure_ss; // contains the code for the actual procedure

    // save current scope info
    std::string previous_scope_name = this->current_scope_name;
    unsigned int previous_scope_level = this->current_scope_level;
    size_t previous_max_offset = this->max_offset;

    // todo: sub from rsp the width of the formal parameters
    // todo: ensure stack-passed parameters get written to the proper location

    // update the scope information
    this->current_scope_name = definition.get_name();
    this->current_scope_level = 1;
    this->max_offset = 0;

    // construct the symbol for the function -- everything is offloaded to the utility
    function_symbol func_sym = create_function_symbol(definition);

    // todo: delete -- no longer relevant due to calling convention updates
    // // now, update the stack offset to account for all parameters
    // if (!func_sym.get_formal_parameters().empty()) {
    //     this->max_offset += func_sym.get_formal_parameters()[func_sym.get_formal_parameters().size() - 1].get_offset();
    // }

    // add the symbol to the table
    this->add_symbol(func_sym, definition.get_line_number());

    // now, we have to iterate over the function symbol's parameters and add them to our symbol table
    // todo: optimize by enabling symbol table additions in template function?
    std::set<reg> arg_regs;
    for (symbol &sym: func_sym.get_formal_parameters()) {
        // add the parameter symbol to the table
        this->add_symbol(sym, definition.get_line_number());
        
		// if r was passed in a register, then we must add it to arg_regs
		reg r = sym.get_register();
        if (r != NO_REGISTER) {
            arg_regs.insert(sym.get_register());
        }
    }

    // todo: delete - no longer relevant due to calling convention updates
    // // update the stack offset -- since symbols are pushed in order, just get the last one and add it to the current offset
    // if (!func_sym.get_formal_parameters().empty()) {
    //     const symbol &last_sym = func_sym.get_formal_parameters().back(); 
    //     this->max_offset += last_sym.get_data_type().get_width() + last_sym.get_offset();
    // }

    // get the register_usage object from func_sym and push that
    this->reg_stack.push_back(func_sym.get_arg_regs());

    // add a label for the function
    definition_ss << func_sym.get_name() << ":" << std::endl;
    // note: we don't need to account for parameters passed in registers as these will be located *above* the return address

    // since we will be using the 'call' instruction, we must increase our stack offset by the width of a pointer so that we don't overwrite the return address
    // we don't need to adjust RSP manually, though, as that was done by the "call" instruction
    this->max_offset += sin_widths::PTR_WIDTH;

    // now, compile the procedure using compiler::compile_ast, passing in this function's signature
    procedure_ss = this->compile_ast(*definition.get_procedure().get(), std::make_shared<function_symbol>(func_sym));

    // now, put everything together in definition_ss by adding procedure_ss onto the end
    definition_ss << procedure_ss.str() << std::endl;

    // restore our scope information (except our register stack -- it was already popped)
    this->current_scope_name = previous_scope_name;
    this->current_scope_level = previous_scope_level;
    this->max_offset = previous_max_offset;

    return definition_ss;
}

template std::stringstream compiler::call_function(Call, unsigned int, bool);
template std::stringstream compiler::call_function(ValueReturningFunctionCall, unsigned int, bool);

template<typename T>
std::stringstream compiler::call_function(T call, unsigned int line, bool allow_void) {
    /*

    call_function
    Generates the code to call a function

    Performs all of the necessary functions for the caller in the SIN calling convention
    This includes setting up the call stack, calling the function, and performing any necessary compiler object updates
    Note that this function delegates to the appropriate code generation function based on what the calling convention is

    Note that the return value will always be moved into either RAX or XMM0, depending on its type, whatever the calling convention is
    
    For more information on compiler calling conventions, see doc/Calling Convention.md

    @param  call    May either be a Call statement or a ValueReturningFunctionCall; it contains the parsed function information
    @param  line    The line number on which the call occurs
    @return A stringstream containing the generated code
    @throws This function will throw an exception if an error is generated during the call

    */

    std::stringstream call_ss;

    // first, look up the function
    std::shared_ptr<symbol> sym = this->lookup(call.get_func_name(), line);

    // ensure we have a function
    if (sym->get_symbol_type() == FUNCTION_SYMBOL) {
        // cast to the correct type
        function_symbol func_sym = *dynamic_cast<function_symbol*>(sym.get());

        // if we aren't allowing a void return type, then throw an exception if the primary type is void
        if (!allow_void && func_sym.get_data_type().get_primary() == VOID) {
            throw VoidException(line);
        }

        // behaves according to the calling convention
        if (func_sym.get_calling_convention() == calling_convention::SINCALL) {
            // SIN calling convention
            call_ss << this->sincall(func_sym, call.get_args(), line).str(); // todo: return this function's result directly?
		}
		else if (func_sym.get_calling_convention() == calling_convention::SYSTEM_V) {
			// todo: System V
		}
        else if (func_sym.get_calling_convention() == calling_convention::WIN_64) {
            // todo: Windows x86-64
        }
		else {
            throw CompilerException("Other calling conventions not supported at this time", 0, line);
        }

        // now, the function's return value (or pointer to the return value) is in RAX or XMM0, depending on the type -- the compiler always expects return values here regardless of calling convention
        // further, all clean-up has been handled, so we can continue code generation
    } else {
        throw InvalidSymbolException(line);
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
    @throws Throws an exception if the function signature does not match the arguments supplied

    */

    std::stringstream register_preservation_ss; // for preserving registers used for argument passing
    std::stringstream sincall_ss;

    // todo: if registers need to be preserved, that should be done here

    // get the formal parameters so we don't need to call a function every time
    std::vector<symbol> &formal_parameters = s.get_formal_parameters();

    // ensure the number of arguments provided is less than or equal to the number expected
    if (args.size() <= s.get_formal_parameters().size()) {
        // get the width of arguments so we can calculate the offsets
        unsigned int total_offset = 0;
        for (symbol& s: formal_parameters) {
            total_offset += s.get_data_type().get_width();
        }

        // we only need to subtract from rsp if the adjustment is non-zero
        // do this now so that pushing values to the stack in evaluate_expression() doesn't overwrite parameters
        if (total_offset != 0) {
            sincall_ss << "\t" << "sub rsp, " << total_offset << std::endl;
        }

        // iterate over our arguments, ensure the types match and that we have an appropriate number
        unsigned int rsp_offset_adjust = 0;
        for (size_t i = 0; i < args.size(); i++) {
            // get the argument and its corresponding symbol
            std::shared_ptr<Expression> arg = args[i];
            symbol param = formal_parameters[i];

            // first, ensure the types match
            DataType arg_type = get_expression_data_type(arg, this->symbols, this->structs, line);
            if (arg_type.is_compatible(param.get_data_type())) {
                // evaluate the expression and pass it in the appropriate manner
                sincall_ss << this->evaluate_expression(arg, line).str();
                std::string reg_name = get_rax_name_variant(param.get_data_type(), line);

                // now, determine where that data should go -- this has been determined already so we don't need to do it on every function call
                // if the symbol has a register, pass it there; else, push it
                if (param.get_register() == NO_REGISTER) {
                    // use [rsp + (offset - BASE_PARAMETER_OFFSET)] because we haven't used pushfq or push rbp yet
                    // however, the stack pointer is still below where we want to write the data
                    // if the data needs to be copied in, we must handle it a little different
                    if (param.get_data_type().get_primary() == ARRAY) {
                        // todo: perform array copy
                        // todo: semantics for passing arrays into functions
                    }
                    else if (param.get_data_type().get_primary() == STRING) {
                        // todo: perform string copy
                        // if the string is final, we don't need to perform a string copy -- we can use the address
                        if (param.get_data_type().get_qualities().is_final()) {
                            sincall_ss << "\t" << "mov [rsp + " << -param.get_offset() - general_utilities::BASE_PARAMETER_OFFSET << "], " << reg_name << std::endl;
                        }
                    }
                    else if (param.get_data_type().get_primary() == STRUCT) {
                        // todo: struct assignment
                    }
                    else {
                        // since we are using +, we don't need to negate the parameter offset (as we normally would if we were using -)
                        sincall_ss << "\t" << "mov [rsp + " << param.get_offset() - general_utilities::BASE_PARAMETER_OFFSET << "], " << reg_name << std::endl;
                    }
                } else {
                    sincall_ss << "\t" << "mov " << register_usage::get_register_name(param.get_register(), param.get_data_type()) << ", " << reg_name << std::endl;
                }

                // now, modify the rsp offset adjustment -- must be done for all arguments as they all live above the new stack frame
                rsp_offset_adjust += param.get_data_type().get_width();
            } else {
                // if the types don't match, we have a signature mismatch
                throw FunctionSignatureException(line);
            }
        }

        // preserve the processor status
        sincall_ss << "\t" << "pushfq" << std::endl;
        // now, preserve our stack frame
        sincall_ss << "\t" << "push rbp" << std::endl;
        sincall_ss << "\t" << "mov rbp, rsp" << std::endl;

        // next, call the function
        sincall_ss << "\t" << "call " << s.get_name() << std::endl;

        // the return value is now in RAX or XMM0, depending on the data type

        // now, restore the old stack frame
        sincall_ss << "\t" << "mov rsp, rbp" << std::endl;
        sincall_ss << "\t" << "pop rbp" << std::endl;
        // and restore the processor status
        sincall_ss << "\t" << "popfq" << std::endl;

        // if we had to adjust rsp, move it back
        if (total_offset != 0) {
            sincall_ss << "\t" << "add rsp, " << total_offset << std::endl;
        }

        // todo: if registers were preserved, restore them here
    } else {
        // If the number of arguments supplied exceeds the number expected, throw an error -- the call does not match the signature
        throw FunctionSignatureException(line);
    }

    // finally, return our call code
    return sincall_ss;
}

std::stringstream compiler::system_v_call(function_symbol s, std::vector<std::shared_ptr<Expression>> args, unsigned int line)
{
    std::stringstream system_v_call_ss;

    // todo: System V ABI call

    return system_v_call_ss;
}

std::stringstream compiler::win64_call(function_symbol s, std::vector<std::shared_ptr<Expression>> args, unsigned int line)
{
    std::stringstream win64_call_ss;

    // todo: Windows 64 call

    return win64_call_ss;
}

// Function returns

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
    DataType return_type = get_expression_data_type(ret.get_return_exp(), this->symbols, this->structs, ret.get_line_number());
    if (return_type.is_compatible(signature.get_data_type())) {
        // types are compatible; how the value gets returned (and how the callee gets cleaned up) depends on the function's calling convention
        if (signature.get_calling_convention() == SINCALL) {
            ret_ss << this->sincall_return(ret, return_type).str() << std::endl;
        }
        else if (signature.get_calling_convention() == SYSTEM_V) {
            // todo: System V
        }
        else if (signature.get_calling_convention() == WIN_64) {
            // todo: Windows 64
        }
        else {
            throw CompilerException("Calling conventions other than sincall are currently not supported", 0, ret.get_line_number());
        }
    } else {
        throw ReturnMismatchException(ret.get_line_number());
    }

    ret_ss << "\t" << "mov rsp, rbp" << std::endl;
    
    // adjust the offset by one pointer width, as rsp needs to be where it was when we pushed the function return value
    ret_ss << "\t" << "sub rsp, " << sin_widths::PTR_WIDTH << std::endl;
    
    // now that the calling convention's return responsibilities have been dealt with, we can return
    ret_ss << "\t" << "ret" << std::endl;
    this->max_offset -= 8;
    return ret_ss;
}

std::stringstream compiler::sincall_return(ReturnStatement &ret, DataType return_type) {
    /*

    sincall_return
    Handles a return statement for a function using the sincall calling convention

    This function is _not_ responsible for restoring used registers; it simply evaluates the expression and returns it according to the calling convention.

	All we need to do is evaluate the expression; the result will be in RAX or XMM0 automatically
	Structs, arrays, and strings will pass _pointers_ to these members in RAX; subsequent assignments will use memory copying, as usual

    */

	std::stringstream sincall_ss;
	sincall_ss << evaluate_expression(ret.get_return_exp(), ret.get_line_number()).str() << std::endl;
    sincall_ss << "\t" << "push rax" << std::endl;

    // decrement the rc of all pointers and dynamic memory
    sincall_ss << decrement_rc(this->symbols, this->current_scope_name, this->current_scope_level, true).str();
    sincall_ss << "\t" << "pop rax" << std::endl;

    return sincall_ss;
}
