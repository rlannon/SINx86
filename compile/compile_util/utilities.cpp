/*

SIN Toolchain (x86 target)
compile_util/utilities.cpp

Some utility functions for the compiler

*/

#include "utilities.h"

namespace function_util {
    std::stringstream call_sre_free(symbol& s);
    std::stringstream call_sre_add_ref(symbol& s);
    std::stringstream call_sre_mam_util(symbol& s, std::string func_name);
    std::string call_sre_function(std::string func_name);

    template<typename T>
    function_symbol create_function_symbol(
        const T& def,
        bool mangle=true,
        bool defined=true,
        const std::string& scope_name = "global",
        unsigned int scope_level = 0, 
        bool is_method = false
    );
}

bool is_valid_cast(const DataType &old_type, const DataType &new_type) {
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

bool is_subscriptable(const Type t) {
    return (t == ARRAY || t == STRING);
}

std::stringstream cast(const DataType &old_type, const DataType &new_type, const unsigned int line, const bool is_strict) {
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
                // if compiling in strict mode, we must throw an exception
                std::string message{ "Attempting to convert floating-point type to a smaller integral type; potential loss of data" };
                if (is_strict)
                {
                    throw CompilerException(
                        message,
                        compiler_errors::WIDTH_MISMATCH,
                        line
                    );
                }
                else
                {
                    compiler_warning(
                        message,
                        compiler_errors::WIDTH_MISMATCH,
                        line
                    );
                }
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
                // if compiling in strict mode, throw an exception
                std::string message{ "Potential data loss when converting integer to floating-point number of smaller width" };
                if (is_strict)
                {
                    throw CompilerException(message, compiler_errors::WIDTH_MISMATCH, line);
                }
                else
                {
                    compiler_warning(
                        message,
                        compiler_errors::WIDTH_MISMATCH,
                        line
                    );
                }
            }
            
            // now that the value is in RAX, use convert signed integer to scalar single/double
            std::string instruction = (new_type.get_qualities().is_long()) ? "cvtsi2sd" : "cvtsi2ss";
            cast_ss << "\t" << instruction << " xmm0, " << reg_name << std::endl;
        }
    }
    else if (new_type.get_primary() == CHAR && old_type.get_primary() == INT) {
        // only integer types may be cast to char
		
		// warn that only the lowest byte will be considered
		if (old_type.get_width() > new_type.get_width())
        {
            std::string message{ "Only the lowest byte will be considered when casting integral types to char" };
            if (is_strict)
            {
                throw CompilerException(
                    message,
                    compiler_errors::WIDTH_MISMATCH,
                    line
                );
            }
            else
            {
			    compiler_warning(message, compiler_errors::WIDTH_MISMATCH, line);
            }
        }
		// we don't actually need any assembly here
    }
    else {
        // invalid cast
        throw InvalidTypecastException(line);
    }

    return cast_ss;
}

bool can_pass_in_register(const DataType& to_check) {
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

std::string get_rax_name_variant(const DataType& t, const unsigned int line) {
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

struct_info define_struct(const StructDefinition &definition, compile_time_evaluator &cte) {
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
                        // set the width
                        if (alloc->get_type_information().is_reference_type())
                            this_width = sin_widths::PTR_WIDTH;
                        else
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
                // set the width
                if (alloc->get_type_information().is_reference_type())
                    this_width = sin_widths::PTR_WIDTH;
                else
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
            const Declaration *decl = static_cast<const Declaration*>(s.get());
            if (decl->is_function()) {
                function_symbol f_sym = function_util::create_function_symbol(*decl, true, true, struct_name, 1, true);
            }
            else {
                // todo: other declarations
            }
        }
        else if (s->get_statement_type() == FUNCTION_DEFINITION) {
            // cast and define the function
            const FunctionDefinition *def = static_cast<const FunctionDefinition*>(s.get());
            function_symbol f_sym = function_util::create_function_symbol(*def, true, true, struct_name, 1, true);
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

template symbol generate_symbol(const Declaration&, const size_t, const std::string&, const unsigned int, size_t&, const bool);
template symbol generate_symbol(const Allocation&, const size_t, const std::string&, const unsigned int, size_t&, const bool);
template <typename T>
symbol generate_symbol(const T &allocation, const size_t data_width, const std::string& scope_name, const unsigned int scope_level, size_t &stack_offset, const bool defined) {
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

    const DataType &type_info = allocation.get_type_information();
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

symbol generate_symbol(
    const DataType &type_information,
    const std::string& symbol_name,
    size_t data_width,
    bool defined,
    const std::string& scope_name,
    unsigned int scope_level,
    size_t& stack_offset,
    unsigned int line_number
) {
    // An overload; this will allow allocation to use the type information it fetches and calculates

    bool mangle = !type_information.get_qualities().is_extern();
    stack_offset += data_width;
    std::string name = mangle ? symbol_table::get_mangled_name(symbol_name) : symbol_name;
    symbol to_return(
        symbol_name,
        scope_name,
        scope_level,
        type_information,
        stack_offset,
        defined,
        line_number
    );

    return to_return;
}

std::stringstream store_symbol(const symbol &s) {
    /*

    store_symbol
    Store symbol in its memory location

    */

    std::stringstream store_ss;

    const DataType& dt = s.get_data_type();
    std::string store_instruction;
    if (dt.get_primary() == FLOAT) {
        store_instruction = (dt.get_qualities().is_long()) ? "movsd" : "movss";
    }
    else {
        store_instruction = "mov";
    }

    if (dt.get_qualities().is_static()) {
        store_ss << "\t" << "lea rax, [" << s.get_name() << "]" << std::endl;
        store_ss << "\t" << store_instruction << " [rax], " << register_usage::get_register_name(s.get_register(), dt) << std::endl;
    }
    else if (dt.get_qualities().is_dynamic()) {
        store_ss << "\t" << "mov rax, [rbp - " << s.get_offset() << "]" << std::endl;
        store_ss << "\t" << store_instruction << " [rax], " << register_usage::get_register_name(s.get_register(), dt) << std::endl;
    }
    else {
        store_ss << "\t" << store_instruction << " [rbp - " << s.get_offset() << "], " << register_usage::get_register_name(s.get_register(), dt) << std::endl;
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

std::stringstream pop_used_registers(const register_usage& regs, bool ignore_ab) {
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

std::string get_address(const symbol &s, const reg r) {
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
        if (s.get_register() != r)
            address_info = "\tmov " + reg_name + ", " + register_usage::get_register_name(s.get_register()) + "\n";
    }

    return address_info;
}

std::string get_struct_member_address(
    const symbol &struct_symbol,
    struct_table &structs,
    const std::string &member_name,
    const reg r
) {
    /*

    get_address
    Gets the address of a struct member

    */

    std::stringstream get_ss;

    auto &si = structs.find(struct_symbol.get_data_type().get_struct_name(), 0);
    symbol *member = si.get_member(member_name);
    if (member) {
        get_ss << get_address(struct_symbol, RAX);
        get_ss << "\t" << "add rax, " << member->get_offset() << std::endl;
        get_ss << "\t" << "mov " << register_usage::get_register_name(r) << ", [rax]" << std::endl;
    }
    else {
        throw SymbolNotFoundException(0);
    }

    return get_ss.str();
}

std::string decrement_rc(
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

    // preserve registers
    dec_ss << "\t" << "pushfq" << std::endl;
    dec_ss << push_used_registers(r, true).str();

    // get the local variables that need to be freed
    auto v = symbols.get_symbols_to_free(scope, level, is_function);

    // now we need to look at the structs in the scope that we are leaving and see if we need to decrement any of their memebers
    auto local_structs = symbols.get_local_structs(scope, level, is_function);
    for (auto ls: local_structs) {
        struct_info &info = structs.find(ls->get_data_type().get_struct_name(), 0);
        auto struct_members = info.get_members_to_free();
        dec_ss << decrement_rc_util(struct_members, symbols, structs, scope, 1, false, ls);
    }
    // todo: right now, structs cannot contain other structs, but if this feature is added, this function must change to free reference types within /those/ structs (wouldn't get caught here)

    if (!v.empty())
        dec_ss << decrement_rc_util(v, symbols, structs, scope, level, is_function);

    dec_ss << pop_used_registers(r, true).str();
    dec_ss << "\t" << "popfq" << std::endl;

    return dec_ss.str();
}

std::string decrement_rc_util(
    std::vector<symbol> &to_free,
    symbol_table &symbols,
    struct_table &structs,
    std::string scope,
    unsigned int level,
    bool is_function,
    symbol *parent
) {
    /*

    decrement_rc_util
    The function that is called by decrement_rc

    */


    std::stringstream dec_ss;

    for (symbol s: to_free) {
        dec_ss << "; freeing symbol " << s.get_name() << std::endl;
        if (parent) {
            dec_ss << get_struct_member_address(*parent, structs, s.get_name(), RDI);
        }
        else {
            dec_ss << get_address(s, RDI);
        }

        if (s.get_data_type().get_primary() == ARRAY) {
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
            
            if (s.get_data_type().get_subtype().must_free()) {
                // preserve rdi; move rdi into r12, as the array address is now in rdi
                dec_ss << "\t" << "push rdi" << std::endl;
                dec_ss << "\t" << "mov r12, rdi" << std::endl;

                // ensure 16-byte alignment
                dec_ss << "\t" << "mov rax, rsp" << std::endl;
                dec_ss << "\t" << "and rsp, -0x10" << std::endl;
                dec_ss << "\t" << "push rax" << std::endl;
                dec_ss << "\t" << "sub rsp, 0x08" << std::endl;
                dec_ss << "\t" << "mov r13, 0" << std::endl;

                dec_ss << ".free_array_:" << std::endl;
                dec_ss << "\t" << "cmp r13d, [r12]" << std::endl;
                dec_ss << "\t" << "jge .free_array_done_" << std::endl;
                dec_ss << "\t" << "mov rdi, [r12 + r13 * 8 + 4]" << std::endl;
                dec_ss << "\t" << "call " << magic_numbers::SRE_FREE << std::endl;
                dec_ss << "\t" << "inc r13" << std::endl;
                dec_ss << "\t" << "jmp .free_array_" << std::endl;

                // restore original stack alignment
                dec_ss << ".free_array_done_:" << std::endl;
                dec_ss << "\t" << "add rsp, 0x08" << std::endl;
                dec_ss << "\t" << "pop rsp" << std::endl;
                dec_ss << "\t" << "pop rdi" << std::endl;   // restore rdi's original value
            }
            
            // if the array itself must be freed, do so
            if (s.get_data_type().must_free()) {
                dec_ss << function_util::call_sre_function(magic_numbers::SRE_FREE);
            }

        }
        else if (s.get_data_type().get_primary() == TUPLE) {
            // todo: free tuple members

            // if the tuple itself must be freed, do so
            if (s.get_data_type().must_free()) {
                dec_ss << function_util::call_sre_function(magic_numbers::SRE_FREE);
            }
        }
        else {
            dec_ss << function_util::call_sre_function(magic_numbers::SRE_FREE);
        }
    }

    return dec_ss.str();
}
