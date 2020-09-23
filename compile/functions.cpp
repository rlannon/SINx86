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
        function_symbol sym = create_function_symbol(
            decl_stmt,
            !decl_stmt.get_type_information().get_qualities().is_extern(),
            false
        );
        this->add_symbol(sym, decl_stmt.get_line_number());
        if (this->externals.count(sym.get_name())) {
            throw DuplicateDefinitionException(decl_stmt.get_line_number());
        }
        else {
            this->externals.insert(sym.get_name());
        }
    } else if (decl_stmt.is_struct()) {
        // add struct to struct table with the caveat that it's an incomplete type - this means that member access is not possible
        // note 'extern' is not needed here -- no symbol information is created
        struct_info s_info(decl_stmt.get_type_information().get_struct_name());
        this->add_struct(s_info, decl_stmt.get_line_number());
    } else {
        // add a symbol
        // note: pass 0 as the data width because declared data doesn't occupy stack space
        symbol sym = generate_symbol(decl_stmt, 0, this->current_scope_name, this->current_scope_level, this->max_offset, false);
        this->add_symbol(sym, decl_stmt.get_line_number());
        if (this->externals.count(sym.get_name())) {
            throw DuplicateDefinitionException(decl_stmt.get_line_number());
        }
        else {
            this->externals.insert(sym.get_name());
        }
    }

    return decl_ss;
}

std::stringstream compiler::define_function(FunctionDefinition &definition) {
    /*
    
    define_function
    An overloaded version to define a function with a definition statement
    
    */

    function_symbol func_sym = create_function_symbol(definition);
    return this->define_function(
        func_sym,
        definition.get_procedure(),
        definition.get_line_number()
    );
}

std::stringstream compiler::define_function(function_symbol &func_sym, StatementBlock prog, unsigned int line) {
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

    // update the scope information
    this->current_scope_name = func_sym.get_name();
    this->current_scope_level += 1;
    this->max_offset = 0;

    // check to see if the symbol already exists in the table and is undefined -- if so, we need to add 'global'
    bool marked_extern = false; // to ensure we don't mark it as global twice if the declared function is 'extern'
    if (this->symbols.contains(func_sym.get_name())) {
        auto &sym = this->symbols.find(func_sym.get_name());
        if (sym.get_symbol_type() == FUNCTION_SYMBOL) {
            auto &declared_sym = static_cast<function_symbol&>(sym);
            if (sym.is_defined()) {
                throw DuplicateDefinitionException(line);
            }
            else {
                // check to ensure signatures match
                if (
                    !func_sym.matches(declared_sym)
                ) {
                    throw CompilerException(
                        "Signature for '" + func_sym.get_name() + "' does not match that of declaration",
                        compiler_errors::SIGNATURE_MISMATCH,
                        line
                    );
                }

                // mark this label as 'global', delete the 'extern' statement for it in this file
                definition_ss << "global " << func_sym.get_name() << std::endl;
                sym.set_defined();
                marked_extern = true;

                if (this->externals.count(sym.get_name())) {
                    this->externals.erase(sym.get_name());
                }
            }
        }
        else {
            throw CompilerException(
                "Attempt to redefine \"" + func_sym.get_name() + "\" as a function",
                compiler_errors::DUPLICATE_SYMBOL_ERROR,
                line
            );
        }
    }
    else {
        // add the symbol to the table
        this->add_symbol(func_sym, line);
    }

    // if the function is marked as 'extern', ensure it is global
    if (!marked_extern && func_sym.get_data_type().get_qualities().is_extern()) {
        definition_ss << "global " << func_sym.get_name() << std::endl;
    }

    // now, we have to iterate over the function symbol's parameters and add them to our symbol table
    // todo: optimize by enabling symbol table additions in template function?
    std::unordered_map<symbol*, reg> arg_regs;
    for (auto sym: func_sym.get_formal_parameters()) {
        // add the parameter symbol to the table
        symbol &inserted = this->add_symbol(sym, line);
        
		// if r was passed in a register, then we must add it to arg_regs
		reg r = sym->get_register();
        if (r != NO_REGISTER) {
            arg_regs.insert(
                std::make_pair<>(&inserted, sym->get_register())
            );
        }
    }

    // get the register_usage object from func_sym and push that
    this->reg_stack.push_back(func_sym.get_arg_regs());

    // add a label for the function
    definition_ss << func_sym.get_name() << ":" << std::endl;
    // note: we don't need to account for parameters passed in registers as these will be located *above* the return address
    // it is the *caller's* responsibility to allocate this data

    // since we will be using the 'call' instruction, we must increase our stack offset by the width of a pointer so that we don't overwrite the return address
    // we don't need to adjust RSP manually, though, as that was done by the "call" instruction
    this->max_offset += sin_widths::PTR_WIDTH;

    // now, compile the procedure using compiler::compile_ast, passing in this function's signature
    procedure_ss = this->compile_ast(prog, &func_sym);

    // after compiling the AST, we need to restore the registers that our parameter symbols were contained in
    // otherwise, when the function is called, we won't know what registers to pass arguments in
    for (auto it = arg_regs.begin(); it != arg_regs.end(); it++) {
        it->first->set_register(it->second);
    }

    // now, put everything together in definition_ss by adding procedure_ss onto the end
    definition_ss << procedure_ss.str() << std::endl;

    // restore our scope information
    this->current_scope_name = previous_scope_name;
    this->current_scope_level = previous_scope_level;
    this->max_offset = previous_max_offset;
    this->reg_stack.pop_back();

    return definition_ss;
}

std::pair<std::string, size_t> compiler::call_function(Procedure &to_call, unsigned int line, bool allow_void) {
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
    @return A pair containing:
        * a stringstream containing the generated code
        * a size_t with the number of references to free
    @throws This function will throw an exception if an error is generated during the call

    */

    std::stringstream call_ss;
    size_t count = 0;

    // first, look up the function
    symbol &sym = expression_util::get_function_symbol(
        to_call.get_func_name(),
        this->structs,
        this->symbols,
        line
    );

    // if the function returns a reference type, we need to increment the count
    if (sym.get_data_type().is_reference_type()) {
        count = 1;
    }

    // ensure we have a function
    // todo: data types could be valid as well -- proc type
    if (sym.get_symbol_type() == FUNCTION_SYMBOL) {
        // cast to the correct type
        function_symbol &func_sym = static_cast<function_symbol&>(sym);

        // if we aren't allowing a void return type, then throw an exception if the primary type is void
        if (!allow_void && func_sym.get_data_type().get_primary() == VOID) {
            throw VoidException(line);
        }

        // check to see if we have a 'this' parameter that needs evaluation
        if (func_sym.requires_this() && to_call.get_func_name().get_expression_type() == BINARY) {
            Binary &bin_name = static_cast<Binary&>(to_call.get_func_name());
            to_call.insert_arg(bin_name.get_left(), 0);
        }

        // behaves according to the calling convention
        if (func_sym.get_calling_convention() == calling_convention::SINCALL) {
            // SIN calling convention
            call_ss << this->sincall(func_sym, to_call.get_args().get_list(), line).str(); // todo: return this function's result directly?
		}
		else if (func_sym.get_calling_convention() == calling_convention::SYSTEM_V) {
			throw CompilerException(
                "System V calling convention (AMD64) currently unsupported",
                compiler_errors::UNSUPPORTED_ERROR,
                line
            );
		}
        else if (func_sym.get_calling_convention() == calling_convention::WIN_64) {
            throw CompilerException(
                "Windows 64-bit calling convention currently unsupported",
                compiler_errors::UNSUPPORTED_ERROR,
                line
            );
        }
		else {
            throw CompilerException(
                "Other calling conventions not supported at this time",
                compiler_errors::UNSUPPORTED_ERROR,
                line
            );
        }

        // now, the function's return value (or pointer to the return value) is in RAX or XMM0, depending on the type -- the compiler always expects return values here regardless of calling convention
        // further, all clean-up has been handled, so we can continue code generation
    } else {
        throw InvalidSymbolException(line);
    }

    // todo: any miscellaneous clean-up?
    return std::make_pair<>(call_ss.str(), count);
}

std::stringstream compiler::sincall(function_symbol s, std::vector<std::shared_ptr<Expression>> args, unsigned int line) {
    /*

    sincall
    Overloaded version to handle a vector of shared pointers

    */

    std::vector<Expression*> to_pass;
    for (auto elem: args) {
        to_pass.push_back(elem.get());
    }
    return this->sincall(s, to_pass, line);
}

std::stringstream compiler::sincall(function_symbol s, std::vector<Expression*> args, unsigned int line) {
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

    // if registers need to be preserved, do that here
    bool pushed = false;
    if (!this->reg_stack.empty()) {
        pushed = true;
        sincall_ss << push_used_registers(this->reg_stack.peek(), true).str();
    }

    // create a new reg stack for our parameters
    this->reg_stack.push_back(register_usage());

    // get the formal parameters so we don't need to call a function every time
    auto &formal_parameters = s.get_formal_parameters();

    // ensure the number of arguments provided is less than or equal to the number expected
    if (args.size() <= s.get_formal_parameters().size()) {
        // get the width of arguments so we can calculate the offsets
        unsigned int total_offset = 0;
        for (auto s: formal_parameters) {
            total_offset += s->get_data_type().get_width();
        }

        // we only need to subtract from rsp if the adjustment is non-zero
        // do this now so that pushing values to the stack in evaluate_expression() doesn't overwrite parameters
        // and, if we have no parameters, we won't have to do any of this
        if (total_offset != 0) {
            sincall_ss << "\t" << "sub rsp, " << total_offset << std::endl;
        }
        
        // iterate over our arguments, ensure the types match and that we have an appropriate number
        for (size_t i = 0; i < args.size(); i++) {
            // get the argument and its corresponding symbol
            Expression *arg = args.at(i);
            symbol &param = *formal_parameters[i];

            // first, ensure the types match
            DataType arg_type = expression_util::get_expression_data_type(*arg, this->symbols, this->structs, line);
            if (!arg_type.is_compatible(param.get_data_type())) {
                // if the types don't match, we have a signature mismatch
                throw FunctionSignatureException(line);
            }
            
            // evaluate the expression and pass it in the appropriate manner
            auto arg_p = this->evaluate_expression(*arg, line, &arg_type);
            sincall_ss << arg_p.first;

            std::string reg_name = get_rax_name_variant(param.get_data_type(), line);
            auto destination_operand = assign_utilities::fetch_destination_operand(
                param, 
                this->symbols, 
                line,
                RBX,
                true
            );
            bool copy_constructed = true;

            // get the offset (rsp+) for this parameter
            // note if we have to adjust the RC, the position will be one quadword *above* RSP because we push first, then do lea
            size_t param_offset = -param.get_offset() - general_utilities::BASE_PARAMETER_OFFSET;
            if (arg_p.second) {
                param_offset += sin_widths::PTR_WIDTH;
            }

            // if the parameter is marked as dynamic, we need to request a new resource from the SRE
            if (param.get_data_type().get_qualities().is_dynamic()) {
                if (param.get_data_type().get_primary() == STRING || param.get_data_type().get_primary() == ARRAY) {
                    // todo: initial lengths for strings and arrays -- we will allocate space based on the parameter's width
                }
                else {
                    // all other types have a known, fixed width
                    sincall_ss << "\t" << "mov rdi, 0" << std::endl;
                }
            }

            // if we had a dynamic or string type, we have to construct it regardless (pass by value)
            if (param.get_data_type().get_primary() == STRING) {
                // to construct a string, we load the address where the parameter wil be stored into rdi
                sincall_ss << "\t" << "lea rdi, [rsp + " << param_offset << "]" << std::endl;
                sincall_ss << push_used_registers(this->reg_stack.peek(), true).str();
                sincall_ss << "\t" << "mov rsi, rax" << std::endl;
                sincall_ss << call_sincall_subroutine("sinl_string_copy_construct") << std::endl;
                sincall_ss << pop_used_registers(this->reg_stack.peek(), true).str();
            }
            else if (arg_type.get_qualities().is_dynamic()) {
                // todo: copy-construct other types
            }
            else {
                copy_constructed = false;
            }

            // if we needed to adjust the RC
            if (arg_p.second) {
                sincall_ss << "\t" << "pop rdi" << std::endl;   // free the original string to free
                sincall_ss << call_sre_function(magic_numbers::SRE_FREE);
                // now we need to move the parameter position back because we popped the value
                param_offset -= sin_widths::PTR_WIDTH;
            }

            // if the symbol has a register, pass it there; else, push it
            if (param.get_register() == NO_REGISTER) {
                if (copy_constructed) {
                    sincall_ss << "\t" << "mov rax, [rsp + " << param_offset << "], " << std::endl;
                }

                // note final strings *can* just copy the reference
                sincall_ss << "\t" << "mov [rsp + " << param_offset << "]" << ", " << reg_name << std::endl;
            } else {
                // ensure the register is set as 'in use'
                if (copy_constructed) {
                    reg_name = "[rsp + " + std::to_string(param_offset) + "]";
                }

                this->reg_stack.peek().set(param.get_register());
                sincall_ss << "\t" << "mov " << register_usage::get_register_name(param.get_register(), param.get_data_type()) << ", " << reg_name << std::endl;
            }

            param.set_initialized();
        }
        // todo: default values

        // call the function
        sincall_ss << call_sincall_subroutine(s.get_name());

        // the return value is now in RAX or XMM0, depending on the data type

        // if we had to adjust rsp, move it back
        if (total_offset != 0) {
            sincall_ss << "\t" << "add rsp, " << total_offset << std::endl;
        }

        // pop back the parameter register_usage object
        this->reg_stack.pop_back();

        // if registers were preserved, restore them here
        if (pushed)
            sincall_ss << pop_used_registers(this->reg_stack.peek(), true).str();
    }
    else {
        // If the number of arguments supplied exceeds the number expected, throw an error -- the call does not match the signature
        throw FunctionSignatureException(line);
    }

    // finally, return our call code
    return sincall_ss;
}

std::stringstream compiler::system_v_call(function_symbol s, std::vector<Expression*> args, unsigned int line)
{
    std::stringstream system_v_call_ss;

    // todo: System V ABI call

    return system_v_call_ss;
}

std::stringstream compiler::win64_call(function_symbol s, std::vector<Expression*> args, unsigned int line)
{
    std::stringstream win64_call_ss;

    // todo: Windows 64 call

    return win64_call_ss;
}

// Function returns

std::stringstream compiler::handle_return(ReturnStatement &ret, function_symbol &signature) {
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
    DataType return_type = expression_util::get_expression_data_type(ret.get_return_exp(), this->symbols, this->structs, ret.get_line_number());
    if (return_type.is_compatible(signature.get_data_type())) {
        // ensure we have a valid return type; we can't return local references
        if (signature.get_data_type().get_primary() == REFERENCE || signature.get_data_type().get_primary() == PTR) {
            DataType returned_subtype = return_type.get_subtype();
            if (!returned_subtype.get_qualities().is_dynamic() && !returned_subtype.get_qualities().is_static()) {
                throw CompilerException(
                    "References to automatic memory may not be returned",
                    compiler_errors::RETURN_AUTOMATIC_REFERENCE,
                    ret.get_line_number()
                );
            }
        }

        // types are compatible; how the value gets returned (and how the callee gets cleaned up) depends on the function's calling convention
        if (signature.get_calling_convention() == SINCALL) {
            ret_ss << this->sincall_return(ret, return_type).str() << std::endl;
        }
        /*else if (signature.get_calling_convention() == SYSTEM_V) {
            // todo: System V
        }
        else if (signature.get_calling_convention() == WIN_64) {
            // todo: Windows 64
        }*/
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

    auto ret_p = this->evaluate_expression(ret.get_return_exp(), ret.get_line_number());

	sincall_ss << ret_p.first;
    // todo: count
    sincall_ss << "\t" << "push rax" << std::endl;
    
    // if we are returning a pointer or address, we need to increment the RC by one so it doesn't get freed completely
    auto t = expression_util::get_expression_data_type(
        ret.get_return_exp(),
        this->symbols,
        this->structs,
        ret.get_line_number()
    );
    if (t.is_reference_type() || t.get_primary() == PTR) {
        sincall_ss << "\t" << "mov rdi, rax" << std::endl;
        sincall_ss << call_sre_function(magic_numbers::SRE_ADD_REF);
    }

    // decrement the rc of all pointers, references, and dynamic memory
    try {
        sincall_ss << decrement_rc(this->reg_stack.peek(), this->symbols, this->structs, this->current_scope_name, this->current_scope_level, true);
    }
    catch (CompilerException &e) {
        e.set_line(ret.get_line_number());  // it would be unusual for this to get caught, but to be safe...
        throw e;
    }

    sincall_ss << "\t" << "pop rax" << std::endl;

    return sincall_ss;
}
