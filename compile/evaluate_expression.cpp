/*

SIN Toolchain (x86 target)
evaluate_expression.cpp

Generates code for evaluating an expression. This data will be loaded into registers as specified by "Doc/Registers"

*/

#include "compiler.h"

// todo: add evaluation of expressions labeled 'constexpr'

// todo: create an expression evaluation class and give it access to compiler members?

std::stringstream compiler::evaluate_expression(std::shared_ptr<Expression> to_evaluate, unsigned int line) {
    /*

    evaluate_expression
    Generates code to evaluate a given expression

    Generates code to evaluate an expression, keeping the result in the A register (could be AL, AX, EAX, or RAX) if possible.
    If the size of the object (i.e., non-dynamic arrays and structs) does not permit it to passed in a register, then it will be returned on the stack, and a pointer to the member will be passed

    Note this function assumes any registers that needed to be saved were, and will be restored by the compiler after the expression evaluation

    @param  value   The expression to be evaluated
    @return A stringstream containing the generated code

    */

    std::stringstream evaluation_ss;

    // The expression evaluation depends on the expression's type
    switch (to_evaluate->get_expression_type()) {
        case LITERAL:
        {
            // get the literal
            Literal literal_exp = *dynamic_cast<Literal*>(to_evaluate.get());

            // dispatch to our evaluation function
            evaluation_ss = this->evaluate_literal(literal_exp, line);
            break;
        }
        case LVALUE:
        {
            // get the lvalue
            LValue lvalue_exp = *dynamic_cast<LValue*>(to_evaluate.get());

            // dispatch to our evaluation function
            evaluation_ss = this->evaluate_lvalue(lvalue_exp, line);
            break;
        }
        case INDEXED:
        {
            // get the indexed expression
            Indexed indexed_exp = *dynamic_cast<Indexed*>(to_evaluate.get());
            
            // dispatch appropriately; evaluation of indexed expressions is sufficiently complicated to warrant it
			evaluation_ss = this->evaluate_indexed(indexed_exp, line);
            break;
        }
        case LIST:
        {
			// todo: evaluate list expressions (using pointers)
            break;
        }
        case ADDRESS_OF:	// todo: move implementation of pointer-related expressions into separate functions
        {
			AddressOf addr_exp = *dynamic_cast<AddressOf*>(to_evaluate.get());
			
			// how we generate code for this depends on the type
            if (addr_exp.get_target()->get_expression_type() == BINARY) {
                // if we have a binary expression, it *must* be the dot operator; if so, just return what's in RBX after we evaluate it
                Binary* target = dynamic_cast<Binary*>(addr_exp.get_target().get());
                if (target->get_operator() != DOT) {
                    throw CompilerException("Illegal binary operand in address-of expression", compiler_errors::ILLEGAL_ADDRESS_OF_ARGUMENT, line);
                }

                evaluation_ss << this->evaluate_binary(*target, line).str();
                evaluation_ss << "mov rax, rbx" << std::endl;
                this->reg_stack.peek().clear(RBX);  // now we can use RBX again
            } else if (addr_exp.get_target()->get_expression_type() == LVALUE) {
                LValue* target = dynamic_cast<LValue*>(addr_exp.get_target().get());
                // look up the symbol; obtain the address based on its memory location
                std::shared_ptr<symbol> s = this->lookup(target->getValue(), line);
                if (s->get_data_type().get_qualities().is_static()) {
                    evaluation_ss << "\t" << "mov rax, " << s->get_name() << std::endl;
                } 
                else {
                    // first, get the stack address
                    evaluation_ss << "\t" << "mov rax, rbp" << std::endl;
                    evaluation_ss << "\t" << "sub rax, " << s->get_offset() << std::endl;

                    // if the variable is dynamic or a string, dereference RAX
                    if (s->get_data_type().get_qualities().is_dynamic() || s->get_data_type().get_primary() == STRING) {
                        evaluation_ss << "\t" << "mov rax, [rax]" << std::endl;
                    }
                }
            } else {
                throw CompilerException("Illegal address-of argument", compiler_errors::ILLEGAL_ADDRESS_OF_ARGUMENT, line);
            }

            break;
        }
        case DEREFERENCED:
        {
			// todo: evaluate dereferenced expressions
            break;
        }
        case BINARY:
        {
			// cast to Binary class and dispatch appropriately
			Binary bin_exp = *dynamic_cast<Binary*>(to_evaluate.get());

			// if we have a dot operator, use a separate function; it must be handled slightly differently than other binary expressions
			if (bin_exp.get_operator() == exp_operator::DOT) {
				// create the member_selection object from the expression so it can be evaluated
				member_selection m = member_selection::create_member_selection(bin_exp, this->structs, this->symbols, line);

				// before we evaluate it, check to see whether the struct was initialized; since individual members are not allocated, if *any* member is assigned, the symbol is considered initialized
				if (!m.first().was_initialized())
					throw ReferencedBeforeInitializationException(m.last().get_name(), line);

				// now, generate the code to get our data member
				evaluation_ss << this->evaluate_member_selection(m, line).str();

                // now, the address of our data is in RBX; dereference accordingly
                if (can_pass_in_register(m.last().get_data_type())) {
                    evaluation_ss << "\t" << "mov " << get_rax_name_variant(m.last().get_data_type(), line) << ", [rbx]" << std::endl;
                } else {
                    // if it can't pass in a register, move the pointer into RAX
                    // todo: array and struct members
                }
			}
			else {
				evaluation_ss << this->evaluate_binary(bin_exp, line).str();
			}

			break;
        }
        case UNARY:
        {
			Unary unary_exp = *dynamic_cast<Unary*>(to_evaluate.get());
			evaluation_ss << this->evaluate_unary(unary_exp, line).str();
            break;
        }
        case VALUE_RETURNING_CALL:
        {
            ValueReturningFunctionCall call_exp = *dynamic_cast<ValueReturningFunctionCall*>(to_evaluate.get());
            evaluation_ss << this->call_function(call_exp, line, false).str();  // don't allow void functions here
            break;
        }
        case SIZE_OF:
        {
            SizeOf sizeof_exp = *dynamic_cast<SizeOf*>(to_evaluate.get());
            evaluation_ss = this->evaluate_sizeof(sizeof_exp, line);
            break;
        }
        default:
            throw CompilerException("Invalid expression type", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
    }

    return evaluation_ss;
}

std::stringstream compiler::evaluate_literal(Literal &to_evaluate, unsigned int line) {
    /*

    evaluate_literal
    Evaluates a literal expression

    This function will generate code to put a literal value in the A register, or on the stack if necessary. Note that the value will be in RAX, EAX, AX, or AL depending on the data width.
    This function will add data to the compiler's .data or .rodata section where needed.
	Note that pointer literals are considered 'address-of' expressions and so are not included in this function.

    @param  to_evaluate The literal expression we are evaluating
    @param  line    The line number of the expression (for error handling)
    @return A stringstream containing the generated code

    */

    std::stringstream eval_ss;

    // act based on data type and width
    DataType type = to_evaluate.get_data_type();

    if (type.get_primary() == VOID) {
        // A void literal gets loaded into rax as 0
        // These are used in return statements for void-returning functions
        eval_ss << "\t" << "mov rax, 0" << std::endl;
    } else if (type.get_primary() == INT) {
        /*

        short ints are 16 bits wide and get loaded into ax
        normal-width ints are 32 bits wide and get loaded into eax
        long ints are 64 bits wide and get loaded into rax

        */
        
        if (type.get_width() == sin_widths::SHORT_WIDTH) {
            eval_ss << "\t" << "mov ax, " << to_evaluate.get_value() << std::endl;
        } else if (type.get_width() == sin_widths::INT_WIDTH) {
            eval_ss << "\t" << "mov eax, " << to_evaluate.get_value() << std::endl;
        } else if (type.get_width() == sin_widths::DOUBLE_WIDTH) {
            eval_ss << "\t" << "mov rax, " << to_evaluate.get_value() << std::endl;
        } else {
            throw CompilerException("Invalid type width", 0, line);
        }
    } else if (type.get_primary() == FLOAT) {
        // todo: handle floats
    } else if (type.get_primary() == BOOL) {
        /*

        Booleans get loaded into al; the result is 0 (false) or 1 (true)

        */

        // all other values should get caught by the parser as non-literals, but we will just be extra safe
        if (to_evaluate.get_value() == "true") {
            eval_ss << "\t" << "mov al, 1" << std::endl;
        } else if (to_evaluate.get_value() == "false") {
            eval_ss << "\t" << "mov al, 0" << std::endl;
        } else {
            throw CompilerException("Invalid syntax", 0, line);
        }
    } else if (type.get_primary() == CHAR) {
        // Since SIN uses ASCII (for now), all chars get loaded into al as they are only a byte wide
        if (type.get_width() == sin_widths::CHAR_WIDTH) {
            // NASM supports an argument like 'a' for mov to load the char's ASCII value
            eval_ss << "mov al, '" << to_evaluate.get_value() << "'" << std::endl;
        } else {
            throw CompilerException("Unicode currently not supported", compiler_errors::UNICODE_ERROR, line);
        }
    } else if (type.get_primary() == STRING) {
        /*
        
        Keep in mind that strings are really _pointers_ under the hood -- so we will return a pointer to the string literal, not the string itself on the stack

        However, we need to reserve space for the string in the .rodata segment

        */

        std::string name = "strc_" + std::to_string(this->strc_num);

        // actually reserve the data and enclose the string in backticks in case we have escaped characters
        this->rodata_segment << name << " .dd " << to_evaluate.get_value().length() << " `" << to_evaluate.get_value() << "`" << std::endl;
        this->strc_num += 1;

        // now, load the a register with the address of the string
        eval_ss << "\t" << "mov rax, " << name << std::endl;
    } else if (type.get_primary() == ARRAY) {
        /*

        Array literals are just lists, so this may not be necessary as it may be caught by another function

        */
    } else {
        // invalid data type
        throw TypeException(line);	// todo: enable JSON-style objects to allow struct literals?
    }

    // finally, return the generated code
    return eval_ss;
}

std::stringstream compiler::evaluate_lvalue(LValue &to_evaluate, unsigned int line) {
    /*

    Generate code for evaluating an lvalue (a variable)
    Result will be returned on A, as usual, or on the stack if that is required (depending on data type).

    @param  to_evaluate The lvalue we are evaluating
    @param  line    The line number (for error handling)
    @return A stringstream containing the generated code

    */

    std::stringstream eval_ss;

    // get the symbol for the lvalue; make sure it was initialized
    symbol &sym = *(this->lookup(to_evaluate.getValue(), line).get());
	if (!sym.was_initialized())
		throw ReferencedBeforeInitializationException(sym.get_name(), line);
    
    // check to see if it was freed; we can't know for sure, but if the compiler has it marked as freed, issue a warning that it may have been freed before the reference to it
    if (sym.was_freed())
        compiler_warning("Symbol '" + sym.get_name() + "' may have been freed", line);

    // it must be a variable symbol, not a function definition
    if (sym.get_symbol_type() == FUNCTION_SYMBOL) {
        throw UnexpectedFunctionException(line);
    } else {
        // ensure the symbol is accessible in the current scope
        if (this->is_in_scope(sym)) {
            // mark RAX as 'in use' (if it's already 'in use', this has no effect)
            this->reg_stack.peek().set(RAX);

            // pass differently depending on whether we can pass the argument in a register or if it must be passed on the stack and use a pointer
            if (sym.get_data_type().get_primary() == VOID) {
                // void types should generate a compiler error -- they cannot be evaluated
                throw VoidException(line);
            } else if (can_pass_in_register(sym.get_data_type())) {
                // the data width determines which register size to use
                std::string reg_string = get_rax_name_variant(sym.get_data_type(), line);

                if (sym.get_data_type().get_qualities().is_const()) {
                    // const variables can be looked up by their name -- they are in the .data section
                    eval_ss << "\t" << "mov " << reg_string << ", [" << sym.get_name() << "]" << std::endl;
                } else if (sym.get_data_type().get_qualities().is_static()) {
                    // static memory can be looked up by name -- variables are in the .bss section
                    eval_ss << "\t" << "mov " << reg_string << ", [" << sym.get_name() << "]" << std::endl;
                } else if (sym.get_data_type().get_qualities().is_dynamic()) {
                    // dynamic memory
                    // since dynamic variables are really just pointers, we need to get the pointer and then dereference it

                    // get an unused register; if all are occupied, use rsi (but push it first)
                    reg r;
                    std::string reg_used = "";
                    bool reg_pushed = false;

                    // if there is no register available, use RSI
                    r = this->reg_stack.peek().get_available_register(sym.get_data_type().get_primary());
                    if (r == NO_REGISTER) {
                        // since no register is available, push rsi
                        eval_ss << "\t" << "push rsi" << std::endl;
                        reg_used = "rsi";
                        reg_pushed = true;
                    } else {
                        reg_used = register_usage::get_register_name(r);
                    }

                    // get the dereferenced pointer in A
                    eval_ss << "\t" << "mov " << reg_used << ", [rbp - " << sym.get_offset() << "]" << std::endl;
                    eval_ss << "\t" << "mov " << reg_string << ", [" << reg_used << "]" << std::endl;

                    // if we had to push a register, restore it
                    if (reg_pushed) {
                        eval_ss << "\t" << "pop rsi" << std::endl;
                    }
                } else {
                    // automatic memory
                    // get the stack offset; instruction should be something like
                    //      mov rax, [rbp - 4]
                    eval_ss << "\t" << "mov " << reg_string << ", [rbp - " << sym.get_offset() << "]" << std::endl;
                }

                return eval_ss;
            } else {
                // values too large for registers will use _pointers_, although the SIN syntax hides this fact
                // therefore, all data will go in rax
                if (sym.get_data_type().get_qualities().is_const()) {
                    // const pointers
                    eval_ss << "\t" << "mov rax, " << sym.get_name() << std::endl;
                } else if (sym.get_data_type().get_qualities().is_static()) {
                    // static pointers
                    eval_ss << "\t" << "mov rax, " << sym.get_name() << std::endl;
                } else if (sym.get_data_type().get_qualities().is_dynamic()) {
                    // dynamic memory -- the address of the dynamic memory is on the stack, so we need the offset
                    // get address in A
                    eval_ss << "\t" << "mov rax, [rbp - " << sym.get_offset() << "]" << std::endl;
                } else {
                    /*

                    for automatic memory holding types too big for registers, we want to load the address of where that data lives in the stack
                    so, instead of having something like:
                        mov rax, [rbp - 4]
                    which we would use for something like int or float, we want
                        mov rax, rbp - 4
                    which gives us the address of the data

                    */

                    eval_ss << "\t" << "mov rax, rbp - " << sym.get_offset() << std::endl;
                }
            }
        } else {
            // if the variable is out of scope, throw an exception
            throw OutOfScopeException(line);
        }
    }

	return eval_ss;
}

std::stringstream compiler::evaluate_indexed(Indexed &to_evaluate, unsigned int line) {
    /*

    evaluate_indexed
    Evaluates an indexed expression

    All array indices utilize effective addresses as follows:
		memory_base + index*size + 4
	The program first ensures the array index is within bounds by looking at the 32-bit integer at [array_base]; if we have a valid index, then it fetches the value using the effective address

    @param  to_evaluate The indexed expression we are examining
    @param  line    The line number where the expression occurs (for error handling)
    @return A stringstream containing the generated code

    */
    
    std::stringstream eval_ss;

    // first, get the symbol for the array or string we are indexing; make sure it was initialized
    symbol indexed_sym = *(this->lookup(to_evaluate.getValue(), line).get());
	if (!indexed_sym.was_initialized())
		throw ReferencedBeforeInitializationException(indexed_sym.get_name(), line);

	DataType contained_type = *indexed_sym.get_data_type().get_full_subtype().get();
	size_t full_array_width = indexed_sym.get_data_type().get_array_length() * contained_type.get_width() + 4;

    // ensure it is a valid type to be indexed (array or string); if so, get the index number in ecx
    if (indexed_sym.get_symbol_type() == VARIABLE && 
		(
			indexed_sym.get_data_type().get_primary() == ARRAY || 
			indexed_sym.get_data_type().get_primary() == STRING)
		) {
        // next, get the index and multiply it by the data width to get our byte offset
        eval_ss << this->evaluate_expression(to_evaluate.get_index_value(), line).str();
		
		// preserve the element number in ecx
		eval_ss << "\t" << "mov ecx, eax" << std::endl;	// todo: ensure the index can fit into a 32-bit integer?
        
        // mark RBX and RCX as "in use" in case a future operation requires them
        // this will also cause any function that uses these registers to preserve them when called
        this->reg_stack.peek().set(RBX);
		this->reg_stack.peek().set(RCX);
    } else {
        throw TypeException(line);
    }

	// get the register name so that we can ensure we are reading a value of the appropriate width
	std::string reg_name = get_rax_name_variant(contained_type, line);

    // now that we have the index in RCX, fetch the value
	if (indexed_sym.get_data_type().get_qualities().is_static()) {
		/*
		
		Static arrays can use the name of the variable since they're reserved as named variables prior to runtime
		Note that because they are static, their width must be known at compile time
		
		*/

		eval_ss << "\t" << "mov eax, [" << indexed_sym.get_name() << "]" << std::endl;
		eval_ss << "\t" << "cmp ecx, eax" << std::endl;	// perform the bounds check
		// todo: SIN runtime errors; handle the error if necessary

		// access the element
		eval_ss << "\t" << "mov " << reg_name << ", [" << indexed_sym.get_name() << " + rcx*" << contained_type.get_width() << " + " << sin_widths::INT_WIDTH << "]" << std::endl;
	}
	else if (indexed_sym.get_data_type().get_qualities().is_dynamic()) {
		// Dynamic arrays will use a pointer; their width may be variable

		// first, get the length and perform the bounds check
		eval_ss << "\t" << "mov rax, [rbp - " << indexed_sym.get_offset() << "]" << std::endl;
		eval_ss << "\t" << "mov eax, [rax]" << std::endl;	// get the length of the array
		eval_ss << "\t" << "cmp ecx, eax" << std::endl;	// compare EDX (element's index number) with EAX (number of elements in the array)
		// todo: SIN runtime errors -- run an error routine if we are out of bounds
		
		eval_ss << "\t" << "mov rbx, [rbp - " << indexed_sym.get_offset() << "]" << std::endl;
		eval_ss << "\t" << "mov " << reg_name << ", [rbx + rcx*" << contained_type.get_width() << " + " << sin_widths::INT_WIDTH << "]" << std::endl;
    }
	else {
        /*
		
		Automatic arrays can live on the stack because their width _must_ be known at compile time
		They also follow the convention of all other arrays where the lowest address in the array contains the width and the highest address contains the final element. This allows us to utilize effective addresses

		The algorithm is simple:
			- Get the address of the base element for the array
			- Subtract the element's index from that pointer
			- Dereference the pointer

		*/

		eval_ss << "\t" << "mov rbx, rbp" << std::endl;
		eval_ss << "\t" << "sub rbx, " << full_array_width << std::endl;

		// perform a bounds check
		eval_ss << "\t" << "mov eax, [rbx]" << std::endl;
		eval_ss << "\t" << "cmp ecx, eax" << std::endl;	// compare the index number with the number of elements
		// todo: SRE error routine

		eval_ss << "\t" << "mov " << reg_name << ", [rbx + rcx*" << contained_type.get_width() << " + " << sin_widths::INT_WIDTH << "]" << std::endl;
    }

    // return our generated code
    return eval_ss;
}

std::stringstream compiler::evaluate_sizeof(SizeOf &to_evaluate, unsigned int line) {
    /*
    
    Since sizeof<T> always returns a const unsigned int, it should go into eax
    Use sin_widths{} to fetch sizes

    */

    std::stringstream eval_ss;

    if (to_evaluate.get_type().get_primary() == INT) {
        eval_ss << "\t" << "mov eax, ";
        if (to_evaluate.get_type().get_qualities().is_long()) {
            eval_ss << sin_widths::LONG_WIDTH;
        } else if (to_evaluate.get_type().get_qualities().is_short()) {
            eval_ss << sin_widths::SHORT_WIDTH;
        } else {
            eval_ss << sin_widths::INT_WIDTH;
        }
        
        eval_ss << std::endl;
    } else if (to_evaluate.get_type().get_primary() == FLOAT) {
        eval_ss << "\t" << "mov eax, ";
        if (to_evaluate.get_type().get_qualities().is_long()) {
            eval_ss << sin_widths::DOUBLE_WIDTH;
        } else if (to_evaluate.get_type().get_qualities().is_short()) {
            eval_ss << sin_widths::HALF_WIDTH;
        } else {
            eval_ss << sin_widths::FLOAT_WIDTH;
        }
        
        eval_ss << std::endl;
    } else if (to_evaluate.get_type().get_primary() == BOOL) {
        eval_ss << "\t" << "mov eax, " << sin_widths::BOOL_WIDTH << std::endl;
    } else if (to_evaluate.get_type().get_primary() == PTR) {
        eval_ss << "\t" << "mov eax, " << sin_widths::PTR_WIDTH << std::endl;
	}
	else if (to_evaluate.get_type().get_primary() == STRUCT) {
		// look into compiler table to see if we have a struct
			
		// todo: require struct widths to be known at compile time; any variable-width types must be dynamic or utilize pointers

		struct_info &s_info = this->get_struct_info(to_evaluate.get_type().get_struct_name(), line);
		if (s_info.is_width_known()) {
			eval_ss << "\t" << "mov eax, " << s_info.get_width() << std::endl;
		}
		else {
			throw CompilerException("sizeof<T> cannot be used with this struct type because its width is unknown", compiler_errors::UNDEFINED_ERROR, line);
		}
	} else {
        // sizeof<array> and sizeof<string> are invalid
        throw CompilerException("Invalid argument for sizeof<T>; only fixed-width types can be used", compiler_errors::DATA_WIDTH_ERROR, line);
    }

    return eval_ss;
}
