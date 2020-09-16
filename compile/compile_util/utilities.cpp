/*

SIN Toolchain (x86 target)
compile_util/utilities.cpp

Some utility functions for the compiler

*/

#include "utilities.h"


std::string call_sincall_subroutine(std::string name) {
    // Sets up a stack frame, calls a function, restores frame
    
    std::stringstream call_ss;

    call_ss << "\t" << "pushfq" << std::endl;
    call_ss << "\t" << "push rbp" << std::endl;
    call_ss << "\t" << "mov rbp, rsp" << std::endl;
    call_ss << "\t" << "call " << name << std::endl;
    call_ss << "\t" << "mov rsp, rbp" << std::endl;
    call_ss << "\t" << "pop rbp" << std::endl;
    call_ss << "\t" << "popfq" << std::endl;

    return call_ss.str();
}

bool is_valid_cast(DataType &old_type, DataType &new_type) {
    /*

    is_valid_cast
    Determines whether the attempted typecast (from 'old_type' to 'new_type') is valid

    See docs/Typecasting.md for information on typecasting validity rules

    */

    return !(
        old_type.get_primary() == STRING || 
        old_type.get_primary() == ARRAY || 
        new_type.get_primary() == STRING ||
        new_type.get_primary() == ARRAY ||
        old_type.get_primary() == PTR ||
        new_type.get_primary() == PTR ||
        (old_type.get_primary() == CHAR && new_type.get_primary() != INT)
    );
}

bool is_subscriptable(Type t) {
    return (t == ARRAY || t == STRING);
}

std::stringstream cast(DataType &old_type, DataType &new_type, unsigned int line) {
    /*

    cast
    Casts the data in RAX/XMM0 to the supplied type, returning the data in RAX/XMM0, depending on the return type.

    */

    std::stringstream cast_ss;

    if (old_type == new_type) {
        compiler_note("Typecast appears to have no effect", line);  // todo: allow code
    }
    else if (new_type.get_primary() == BOOL) {
        if (old_type.get_primary() == FLOAT) {
            // use the SSE comparison functions
            std::string instruction = (old_type.get_qualities().is_long()) ? "cmpsd" : "cmpss";
        }
        else {
            // any *non-zero* value is true
            cast_ss << "\t" << "cmp rax, 0x00" << std::endl;
        }
        cast_ss << "\t" << "setne al" << std::endl;
    }
    else if (new_type.get_primary() == INT) {
        if (old_type.get_primary() == FLOAT) {
            // for float conversions, we *should* issue a warning
            if (old_type.get_width() > new_type.get_width()) {
                compiler_warning(
                    "Attempting to convert floating-point type to a smaller integral type; potential loss of data",
                    compiler_errors::WIDTH_MISMATCH,
                    line
                );
            }

            // perform the cast with the SSE conversion functions
            if (old_type.get_qualities().is_long()) {
                cast_ss << "\t" << "cvttsd2si rax, xmm0" << std::endl;
            }
            else {
                cast_ss << "\t" << "cvttss2si eax, xmm0" << std::endl;
            }
        }
        else {
            /*

            If we have a boolean, zero extend to RAX
            If both types are signed, move with a sign extension
                * If the old size is smaller than the new one, we need to use a movsx instruction
                * If the new type is smaller or equal, we do nothing
            If one of the types was signed and the other is not, we do nothing
            
            If the types are the same, warn that the cast has no effect;
            if they are not, but we don't need to do anything, the cast is useful for type checking purposes

            */

            if (old_type.get_primary() == BOOL) {
                cast_ss << "\t" << "cmp al, 0" << std::endl;
                cast_ss << "\t" << "setne al" << std::endl;
                cast_ss << "\t" << "movzx rax, al" << std::endl;
            }
            else if (
                (old_type.get_qualities().is_signed() && new_type.get_qualities().is_signed()) && 
                (old_type.get_width() < new_type.get_width())
            ) {
                // we need to move with sign extension
                cast_ss << "\t" << "movsx " 
                    << register_usage::get_register_name(reg::RAX, new_type) << ", " 
                    << register_usage::get_register_name(reg::RAX, old_type) << std::endl;
            }
        }
    }
    else if (new_type.get_primary() == FLOAT) {
        if (old_type.get_primary() == FLOAT) {
            if (old_type.get_width() < new_type.get_width()) {
                // old < new; convert scalar single to scalar double
                cast_ss << "\t" << "cvtss2sd xmm0, xmm0" << std::endl;
            }
            else if (old_type.get_width() > new_type.get_width()) {
                // old > new; convert scalar double to scalar single
                cast_ss << "\t" << "cvtsd2ss xmm0, xmm0" << std::endl;
            }
        }
        else {
            std::string reg_name = get_rax_name_variant(old_type, line);

            // extend the boolean value to RAX
            if (old_type.get_primary() == BOOL) {
                cast_ss << "\t" << "cmp al, 0" << std::endl;
                cast_ss << "\t" << "setne al" << std::endl;
                cast_ss << "\t" << "movzx rax, al" << std::endl;
            }
            else if (old_type.get_primary() == INT && old_type.get_width() > new_type.get_width()) {
                compiler_warning(
                    "Potential data loss when converting integer to floating-point number of smaller width",
                    compiler_errors::WIDTH_MISMATCH,
                    line
                );
            }
            
            // now that the value is in RAX, use convert signed integer to scalar single/double
            std::string instruction = (new_type.get_qualities().is_long()) ? "cvtsi2sd" : "cvtsi2ss";
            cast_ss << "\t" << instruction << " xmm0, " << reg_name << std::endl;
        }
    }
    else if (new_type.get_primary() == CHAR && old_type.get_primary() == INT) {
        // only integer types may be cast to char
    }
    else {
        // invalid cast
        throw InvalidTypecastException(line);
    }

    return cast_ss;
}

bool can_pass_in_register(DataType to_check) {
    // Checks whether the given DataType can be passed in a register or if it must be passed on the stack

    bool can_pass = false;

    Type primary = to_check.get_primary();
    if (primary == ARRAY || primary == STRUCT || primary == TUPLE) {
        // if the type is dyamic, then we can -- we are really passing in a pointer into the function
        // todo: can we have static parameters?
        can_pass = to_check.get_qualities().is_dynamic();
    } else {
        // if we have numeric, bool, or char types, then we can pass on a register
        can_pass = true;
    }

    return can_pass;
}

std::string get_rax_name_variant(DataType t, unsigned int line) {
	/*
	
	get_register_name
	Gets the name of the variant of RAX that is to hold the data of the given type
	
	*/

	std::string reg_string = "rax";

	if (t.get_width() == sin_widths::BOOL_WIDTH) {
		reg_string = "al";
	}
	else if (t.get_width() == sin_widths::SHORT_WIDTH) {
		reg_string = "ax";
	}
	else if (t.get_width() == sin_widths::INT_WIDTH) {
		reg_string = "eax";
	}

	return reg_string;
}

struct_info define_struct(StructDefinition &definition, compile_time_evaluator &cte) {
    /*
    
    define_struct
    Creates a struct_info object based on a syntax tree for a struct definition

    Since this doesn't actually affect our symbol table, we don't need compiler members
    Note that the caller should ensure that the definition statement occurs within the globa scope -- it is not the responsibility of this function

    @param  definition  The definition statement for the struct
    @return A 'struct_info' object which may be added to the compiler's struct table
    @throws Throws a StructDefinitionException if there are statements other than allocations

    */

    // get the struct's name
    std::string struct_name = definition.get_name();

    // iterate through our definition statements and create symbols for all struct members
    std::vector<std::shared_ptr<symbol>> members;
    size_t current_offset = 0;
    for (auto s: definition.get_procedure().statements_list) {
        size_t this_width = 0;

        // Only allocations are allowed within a struct body
        if (s->get_statement_type() == ALLOCATION) {
            // cast to Allocation and create a symbol
            Allocation *alloc = static_cast<Allocation*>(s.get());

            // first, ensure that the symbol's type is not this struct
            if ((alloc->get_type_information().get_primary() == STRUCT) && (alloc->get_type_information().get_struct_name() == struct_name)) {
                throw CompilerException(
                    "A struct may not contain an instance of itself; use a pointer instead",
                    compiler_errors::SELF_CONTAINMENT_ERROR,
                    alloc->get_line_number()
                );
            }
            // todo: once references are enabled, disallow those as well -- they can't be null, so that would cause infinite recursion, too
            else if (alloc->get_type_information().get_primary() == ARRAY) {
                // arrays must have constant lengths or be dynamic
                if (alloc->get_type_information().get_array_length_expression()) {
                    if (alloc->get_type_information().get_array_length_expression()->is_const()) {
                        size_t array_length = stoul(
                            cte.evaluate_expression(
                                *alloc->get_type_information().get_array_length_expression(),
                                definition.get_name(),
                                1,
                                definition.get_line_number()
                            )
                        );
                        array_length = array_length * alloc->get_type_information().get_subtype().get_width() + sin_widths::INT_WIDTH;
                        alloc->get_type_information().set_array_length(array_length);
                        this_width = array_length;
                    }
                    else {
                        throw NonConstArrayLengthException(definition.get_line_number());
                    }
                }
                else {
                    throw NonConstArrayLengthException(definition.get_line_number());
                }
            }
            else {
                this_width = alloc->get_type_information().get_width();
            }

            symbol sym(alloc->get_name(), struct_name, 1, alloc->get_type_information(), current_offset);
            
            // todo: allow default values (alloc-init syntax) in structs
            // to do this, the function might have to be moved out of "utilities" and into "compiler"

            // add that symbol to our vector
            members.push_back(std::make_shared<symbol>(sym));

            // update the data offset
            // todo: handle struct and array members
            current_offset += this_width;
        }
        else if (s->get_statement_type() == DECLARATION) {
            Declaration *decl = static_cast<Declaration*>(s.get());
            if (decl->is_function()) {
                function_symbol f_sym = create_function_symbol(*decl, true, true, struct_name, 1, true);
            }
            else {
                // todo: other declarations
            }
        }
        else if (s->get_statement_type() == FUNCTION_DEFINITION) {
            // cast and define the function
            FunctionDefinition *def = static_cast<FunctionDefinition*>(s.get());
            function_symbol f_sym = create_function_symbol(*def, true, true, struct_name, 1, true);
            members.push_back(std::make_shared<function_symbol>(f_sym));
        }
        else if (s->get_statement_type() == STRUCT_DEFINITION) {
            // todo: sub-structs
            throw CompilerException("This feature (structs within structs) is not currently supported", compiler_errors::ILLEGAL_OPERATION_ERROR, s->get_line_number());
        }
        else {
            throw StructDefinitionException(definition.get_line_number());
        }
    }

    // construct and return a struct_info object
    return struct_info(struct_name, members, definition.get_line_number());
}

// Since the declaration and implementation are in separate files, we need to say which types may be used with our template functions

template function_symbol create_function_symbol(FunctionDefinition, bool, bool, std::string, unsigned int, bool);
template function_symbol create_function_symbol(Declaration, bool, bool, std::string, unsigned int, bool);
template <typename T>
function_symbol create_function_symbol(T def, bool mangle, bool defined, std::string scope_name, unsigned int scope_level, bool is_method) {
    /*

    create_function_symbol
    Creates a symbol for a function based on either a definition or a declaration

    This function is responsible for turning the Statement objects containing parameters into symbol objects, but it _does not_ add them to the symbol table (as it is not a member of compiler)

    @param  def The definition or declaration from which to create our symbol
    @return A symbol containing the function signature

    */

    // todo: this function marks the scope name of parameters without mangling
    std::string name = mangle ? symbol_table::get_mangled_name(def.get_name(), scope_name) : def.get_name();
    std::string inner_scope_name = name;
    unsigned int inner_scope_level = scope_level + 1;
    size_t stack_offset = 0;

    // construct our formal parameters
    std::vector<symbol> formal_parameters;

    // if we have a nonstatic method, we need to make sure the first parameter is 'ref<T> this' (unless it was provided -- in which case, validate)
    bool has_this_parameter = false;
    symbol this_parameter(
        "this",
        inner_scope_name,
        inner_scope_level,
        DataType(
            REFERENCE,  // Default type for 'this' is ref< T >
            DataType(
                STRUCT,
                DataType(),
                symbol_qualities(),
                nullptr,
                scope_name
            ),
            symbol_qualities()
        ),
        0,
        true,
        def.get_line_number()
    );
    this_parameter.set_as_parameter();
    this_parameter.set_initialized();

    if (
        is_method && 
        !def.get_type_information().get_qualities().is_static()
        && def.get_formal_parameters().empty()
    ) {
        formal_parameters.push_back(this_parameter);
        has_this_parameter = true;
    }

    // now, determine which registers can hold which parameters
    for (size_t i = 0; i < def.get_formal_parameters().size(); i++) {
        // get the parameter
        auto param = def.get_formal_parameters().at(i);

        // create the symbol based on our statement
        symbol param_sym;

        // cast to the appropriate symbol type
        if (param->get_statement_type() == DECLARATION) {
            Declaration *param_decl = static_cast<Declaration*>(param);
            param_sym = generate_symbol(
                *param_decl,
                param_decl->get_type_information().get_width(),
                inner_scope_name,
                inner_scope_level,
                stack_offset
            );
        } else if (param->get_statement_type() == ALLOCATION) {
            Allocation *param_alloc = static_cast<Allocation*>(param);
            DataType t = param_alloc->get_type_information();
            param_sym = generate_symbol(
                *param_alloc,
                t.get_width(),
                inner_scope_name,
                inner_scope_level,
                stack_offset
            );
        } else {
            // todo: remove? these errors should be caught by the parser
            throw CompilerException("Invalid statement type in function signature", compiler_errors::ILLEGAL_OPERATION_ERROR, def.get_line_number());
        }

        // ensure the first parameter is 'this' if we need it
        if (i == 0 && is_method && !has_this_parameter) {
            // ensure we have a typename of 'this', make sure it's the right type
            if (param_sym.get_name() == "this") {
                auto t = param_sym.get_data_type();
                if (
                    (
                        t.get_primary() == REFERENCE 
                        || t.get_primary() == PTR
                    ) 
                    && t.get_subtype() == STRUCT 
                    && t.get_struct_name() == scope_name
                ) {
                    has_this_parameter = true;
                }
                else {
                    throw CompilerException(
                        "Expected 'this' parameter to have type of ptr< " + scope_name + " > or ref< " + scope_name + " >",
                        compiler_errors::INCORRECT_THIS_TYPE,
                        def.get_line_number()
                    );
                }
            }
            else if (!def.get_type_information().get_qualities().is_static()) {
                // we need to add a 'this' parameter if it's a nonstatic method
                formal_parameters.push_back(this_parameter);
                has_this_parameter = true;
            }
        }

        // make sure it's marked as a paramter and marked as initialized (so that we don't get errors about uninitialized data in the function)
        param_sym.set_as_parameter();
        param_sym.set_initialized();
        formal_parameters.push_back(param_sym);
    }

    // now, if we have a 'this' parameter and it's a static method, it's an error
    if (has_this_parameter && def.get_type_information().get_qualities().is_static()) {           
        throw CompilerException(
            "Cannot have 'this' parameter for static member functions",
            compiler_errors::ILLEGAL_THIS_PARAMETER,
            def.get_line_number()
        );
    }

    // construct the object
    function_symbol to_return(
        name,
        def.get_type_information(),
        formal_parameters,
        scope_name,
        scope_level,
        def.get_calling_convention(),
        defined,
        def.get_line_number()
    );

    // finally, return the function symbol
    return to_return;
}

template symbol generate_symbol(Declaration&, size_t, std::string, unsigned int, size_t&, bool);
template symbol generate_symbol(Allocation&, size_t, std::string, unsigned int, size_t&, bool);
template <typename T>
symbol generate_symbol(T &allocation, size_t data_width, std::string scope_name, unsigned int scope_level, size_t &stack_offset, bool defined) {
    /*

    generate_symbol
    Creates a symbol for a variable based on its allocation/declaration

    This template function is responsible for creating symbols based on Allocation or Declaration objects. Note that this does not handle the actual allocation of the variable, it just constructs the symbol based on the name, scope, etc. As such, whether the variable was initialized does not matter.
    This will also update the stack offset passed into it to account for the width of the symbol.

    Note this function updates the stack offset *before* it creates the symbol; this prevents us from overwriting the base pointer.
    For example, when an integer is allocated as the first item in a function, its offset will be rbp-4

    @param  allocation  A Declaration or Allocation statement
    @param  data_width  The size of the data, in bytes, that should be taken up on the stack
    @param  scope_name  The name of the scope where the symbol is located
    @param  scope_level The scope level of the symbol
    @param  stack_offset    The stack offset (in bytes) of the symbol from the stack frame base

    @return The constructed symbol object

    */

    DataType &type_info = allocation.get_type_information();
    bool mangle = !type_info.get_qualities().is_extern();   // don't mangle the name if we have the extern quality set

    stack_offset += data_width;

    std::string name = mangle ? symbol_table::get_mangled_name(allocation.get_name()) : allocation.get_name();
    symbol to_return(
        name,
        scope_name,
        scope_level,
        type_info,
        stack_offset,
        defined,
        allocation.get_line_number()
    );

    return to_return;
}

std::stringstream store_symbol(symbol &s) {
    /*

    store_symbol
    Store symbol in its memory location

    */

    std::stringstream store_ss;

    DataType dt = s.get_data_type();
    if (dt.get_qualities().is_static()) {
        store_ss << "\t" << "lea rax, [" << s.get_name() << "]" << std::endl;
        store_ss << "\t" << "mov [rax], " << register_usage::get_register_name(s.get_register(), dt) << std::endl;
    }
    else if (dt.get_qualities().is_dynamic()) {
        store_ss << "\t" << "mov rax, [rbp - " << s.get_offset() << "]" << std::endl;
        store_ss << "\t" << "mov [rax], " << register_usage::get_register_name(s.get_register(), dt) << std::endl;
    }
    else {
        store_ss << "\t" << "mov [rbp - " << s.get_offset() << "], " << register_usage::get_register_name(s.get_register(), dt) << std::endl;
    }

    return store_ss;
}

std::stringstream push_used_registers(register_usage &regs, bool ignore_ab) {
    /*

    push_used_registers
    Given a register_usage object, push all registers that are *currently* in use

    @param  regs    The object containing which registers are in use
    @param  ignore_ab   Whether we should ignore RAX and RBX when we push the registers; defaults to false

    */

    std::stringstream push_ss;

    for (
        std::vector<reg>::const_iterator it = register_usage::all_regs.begin();
        it != register_usage::all_regs.end();
        it++
    ) {
        if (((*it != RAX && *it != RBX) || !ignore_ab) && regs.is_in_use(*it)) {
            // if the register contains a symbol, store it instead of pushing to the stack
            symbol *s = regs.get_contained_symbol(*it);
            if (s) {
                push_ss << store_symbol(*s).str();
                regs.clear(s->get_register());
                s->set_register(NO_REGISTER);
            }
            else {
                push_ss << "\t" << "push " << register_usage::get_register_name(*it) << std::endl;
            }
        }
    }

    return push_ss;
}

std::stringstream pop_used_registers(register_usage regs, bool ignore_ab) {
    /*

    pop_used_registers
    Pops all registers marked as 'in use' in a register_usage object

    Uses a reverse_iterator to iterate through all_regs in order to find registers

    @param  regs    The registers that are in use that must be restored
    @param  ignore_ab   Whether we ignored RAX and RBX in the push; defaults to false

    */

    std::stringstream pop_ss;

    for (
        std::vector<reg>::const_reverse_iterator it = register_usage::all_regs.rbegin();
        it != register_usage::all_regs.rend();
        it++
    ) {
        if (((*it != RAX && *it != RBX) || !ignore_ab) && regs.is_in_use(*it)) {
            pop_ss << "\t" << "pop " << register_usage::get_register_name(*it) << std::endl;
        }
    }

    return pop_ss;
}

std::string get_address(symbol &s, reg r) {
    /*

    get_address
    Gets the address of the given symbol in the specified register

    */

    std::string address_info = "";
    std::string reg_name = register_usage::get_register_name(r);

    // if the symbol is in a register, move the value into r
    if (s.get_register() == NO_REGISTER) {
        // if it's static, we can just use the name
        if (s.get_data_type().get_qualities().is_static()) {
            address_info = "\tlea " + reg_name + ", [" + s.get_name() + "]\n";
        }
        // otherwise, we need to look in the stack
        else if (s.get_data_type().is_reference_type()) {
            address_info = "\tmov " + reg_name + ", [rbp - " + std::to_string(s.get_offset()) + "]\n";
        }
        else {
            if (s.get_offset() < 0) {
                address_info += "\tlea " + reg_name + ", [rbp + " + std::to_string(-s.get_offset()) + "]\n";
            }
            else {
                address_info += "\tlea " + reg_name + ", [rbp - " + std::to_string(s.get_offset()) + "]\n";
            }
        }
    }
    else {
        address_info = "\tmov " + reg_name + ", " + register_usage::get_register_name(s.get_register()) + "\n";
    }

    return address_info;
}

std::stringstream decrement_rc(
    register_usage &r,
    symbol_table& symbols,
    struct_table &structs,
    std::string scope,
    unsigned int level,
    bool is_function
) {
    /*

    decrement_rc
    Decrements the RC of all local variables

    @param  r   The register_usage object containing available and used registers
    @param  symbols The symbol table to use
    @param  structs The struct table to use
    @param  scope   The name of the scope we are looking in
    @param  level   The level of the scope we are leaving
    @param  is_function If we are in a function, we need to free data that's below the scope level as well

    */

    std::stringstream dec_ss;

    // get the local variables that need to be freed
    auto v = symbols.get_symbols_to_free(scope, level, is_function);

    // now we need to look at the structs in the scope that we are leaving and see if we need to decrement any of their memebers
    auto local_structs = symbols.get_local_structs(scope, level, is_function);
    for (auto ls: local_structs) {
        struct_info &info = structs.find(ls->get_data_type().get_struct_name(), 0);
        v = info.get_members_to_free(v, scope, level);
    }
    // todo: right now, structs cannot contain other structs, but if this feature is added, this function must change to free reference types within /those/ structs (wouldn't get caught here)

    if (!v.empty()) {
        // preserve all registers to ensure the memory locations contain their respective values
        dec_ss << push_used_registers(r, true).str();

        // todo: this process could be a bit more modular

        // preserve our status register
        dec_ss << "\t" << "pushfq" << std::endl;
        for (symbol &s: v) {
            if (s.get_data_type().get_primary() == ARRAY && !s.get_data_type().is_reference_type()) {
                /*

                To free array members, we should iterate in our assembly
                The routine begins by:
                    * Aligning the stack to a 16-byte boundary
                    * Load R12 with the array base
                    * Load R13 with 0 (the current index)
                It proceeds as follows:
                    * If R12 < the length of the array, continue; else, done
                    * Load RDI with [R12 + R13 * 8 + 4]
                    * Call the SRE function
                    * Increment R13
                Finally, the routine ends by restoring the original stack alignment

                Currently, pushing r12 and r13 is unnecessary as we pushed all used register before.

                */

                // ensure 16-byte alignment
                dec_ss << "\t" << "mov rax, rsp" << std::endl;
                dec_ss << "\t" << "and rsp, -0x10" << std::endl;
                dec_ss << "\t" << "push rax" << std::endl;
                dec_ss << "\t" << "sub rsp, 0x08" << std::endl;

                dec_ss << get_address(s, R12);
                dec_ss << "\t" << "mov r13, 0" << std::endl;
                dec_ss << ".free_array_" << std::endl;
                dec_ss << "\t" << "cmp r13d, [r12]" << std::endl;
                dec_ss << "\t" << "jg .free_array_done_" << std::endl;
                dec_ss << "\t" << "mov rdi, [r12 + r13 * 8 + 4]" << std::endl;
                dec_ss << "\t" << "call " << magic_numbers::SRE_FREE << std::endl;
                dec_ss << "\t" << "inc r13" << std::endl;
                dec_ss << "\t" << "jmp .free_array_" << std::endl;

                // restore original stack alignment
                dec_ss << ".free_array_done_" << std::endl;
                dec_ss << "\t" << "add rsp, 0x08" << std::endl;
                dec_ss << "\t" << "pop rsp" << std::endl;
                
                // if the array itself must be freed, do so
                if (s.get_data_type().is_reference_type()) {
                    dec_ss << call_sre_free(s).str();
                }
            }
            else if (s.get_data_type().get_primary() == TUPLE) {
                // todo: free tuple members

                // if the tuple itself must be freed, do so
                if (s.get_data_type().is_reference_type()) {
                    dec_ss << call_sre_free(s).str();
                }
            }
            else {
                dec_ss << call_sre_free(s).str();
            }
        }
        // restore the status
        dec_ss << "\t" << "popfq" << std::endl;

        // restore our registers
        dec_ss << pop_used_registers(r, true).str();
    }

    return dec_ss;
}

std::stringstream call_sre_free(symbol& s) {
    /*

    call_sre_free
    Calls _sre_free on the specified symbol

    */

    return call_sre_mam_util(s, magic_numbers::SRE_FREE);
}

std::stringstream call_sre_add_ref(symbol& s) {
    /*

    call_sre_add_ref
    Calls the function to add a reference for the given symbol

    */

    return call_sre_mam_util(s, magic_numbers::SRE_ADD_REF);
}

std::stringstream call_sre_mam_util(symbol& s, std::string func_name) {
    /*

    call_sre_mam_util
    A utility that actually generates code

    Takes a symbol and the function name and generates code for it
    These functions may be:
        * _sre_add_ref
        * _sre_free

    */

    std::stringstream gen;
    std::stringstream get_addr;

    if (s.get_data_type().get_qualities().is_static()) {
        get_addr << "\t" << "lea rdi, " << s.get_name() << std::endl;
    }
    else if (
        s.get_data_type().get_primary() == PTR ||
        s.get_data_type().get_qualities().is_dynamic()
    ) {
        get_addr << "\t" << "mov rdi, [rbp - " << s.get_offset() << "]" << std::endl;
    }
    else {
         // if we have a negative number for the offset, add it instead
        if (s.get_offset() < 0) {
            get_addr << "\t" << "lea rbx, [rbp + " << -s.get_offset() << "]" << std::endl;
        }
        else {
            get_addr << "\t" << "lea rbx, [rbp - " << s.get_offset() << "]" << std::endl;
        }
        get_addr << "\t" << "mov rdi, [rbx]" << std::endl;
    }

    gen << get_addr.str();
    gen << "\t" << "pushfq" << std::endl;
    gen << call_sre_function(func_name);
    gen << "\t" << "popfq" << std::endl;

    return gen;
}

std::string call_sre_function(std::string func_name) {
    // Calls an SRE function
    std::stringstream call_ss;
    call_ss << "\t" << "mov rax, rsp" << std::endl	// ensure we have 16-byte stack alignment
        << "\t" << "and rsp, -0x10" << std::endl
        << "\t" << "push rax" << std::endl
        << "\t" << "sub rsp, 8" << std::endl;
    call_ss << "\t" << "call " << func_name << std::endl;
    call_ss << "\t" << "add rsp, 8" << std::endl
        << "\t" << "pop rsp" << std::endl;
    
    return call_ss.str();
}
