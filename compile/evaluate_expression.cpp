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
            
            break;
        }
        case LIST:
        {
            break;
        }
        case ADDRESS_OF:
        {
            break;
        }
        case DEREFERENCED:
        {
            break;
        }
        case BINARY:
        {
			// cast to Binary class and dispatch
			Binary bin_exp = *dynamic_cast<Binary*>(to_evaluate.get());
			evaluation_ss << this->evaluate_binary(bin_exp, line).str();
            break;
        }
        case UNARY:
        {
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
    This function will add data to the compiler's .data section if a string literal is used.

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
        
        if (type.get_width() == 2) {
            eval_ss << "\t" << "mov ax, " << to_evaluate.get_value() << std::endl;
        } else if (type.get_width() == 4) {
            eval_ss << "\t" << "mov eax, " << to_evaluate.get_value() << std::endl;
        } else if (type.get_width() == 8) {
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

        // todo: shouldn't all other values get caught by the parser as not literals?
        if (to_evaluate.get_value() == "true") {
            eval_ss << "\t" << "mov al, 1" << std::endl;
        } else if (to_evaluate.get_value() == "false") {
            eval_ss << "\t" << "mov al, 0" << std::endl;
        } else {
            throw CompilerException("Invalid syntax", 0, line);
        }
    } else if (type.get_primary() == CHAR) {
        /*

        Since SIN uses ASCII (for now), all chars get loaded into al as they are only a byte wide

        */

        if (type.get_width() == 1) {
            // NASM supports an argument like 'a' for mov to load the char's ASCII value
            eval_ss << "mov al, '" << to_evaluate.get_value() << "'" << std::endl;
        } else {
            throw CompilerException("Unicode currently not supported", compiler_errors::UNICODE_ERROR, line);
        }
    } else if (type.get_primary() == PTR) {
        /*

        Since we are on x64, all pointers are 64-bit and get loaded into rax

        */

        // todo: a literal pointer is an address-of -- so this case may be entirely unnecessary
    } else if (type.get_primary() == STRING) {
        /*
        
        Keep in mind that strings are really _pointers_ under the hood -- so we will return a pointer to the string literal, not the string itself on the stack

        However, we need to reserve space for the string in the .data segment

        */

        // Reserve space for the string in the .data segment
        // name the constant
        std::string name = "strc_" + std::to_string(this->strc_num);
        // create the .data declaration -- and enclose the string in backticks in case we have escaped characters
        std::stringstream data;
        data << name << " .dd " << to_evaluate.get_value().length() << " `" << to_evaluate.get_value() << "`" << std::endl;
        this->strc_num += 1;

        // now, make sure our strings goes into the DATA section
        this->data_segment << data.str();

        // now, load the a register with the address of the string
        eval_ss << "\t" << "mov rax, " << name << std::endl;
    } else if (type.get_primary() == ARRAY) {
        /*

        Array literals are just lists, so this may not be necessary as it may be caught by another function

        */
    } else if (type.get_primary() == STRUCT) {
        // todo: struct literals are not possible in SIN
        // todo: enable JSON-style objects to allow them?
    } else {
        // invalid data type
        throw TypeException(line);
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

    // get the symbol for the lvalue
    symbol &sym = *(this->lookup(to_evaluate.getValue(), line).get());

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
                std::string reg_string;
                if (sym.get_data_type().get_width() == 1 || sym.get_data_type().get_width() == 8) {
                    reg_string = "al";
                } else if (sym.get_data_type().get_width() == 16) {
                    reg_string = "ax";
                } else if (sym.get_data_type().get_width() == 32) {
                    reg_string = "eax";
                } else if (sym.get_data_type().get_width() == 64) {
                    reg_string = "rax";
                } else {
                    // todo: is this necessary?
                    throw CompilerException("Invalid data width for symbol", compiler_errors::DATA_WIDTH_ERROR, line);
                }

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
                    eval_ss << "\t" << "mov " << reg_used << ", [rbp - " << sym.get_stack_offset() << "]" << std::endl;
                    eval_ss << "\t" << "mov " << reg_string << ", [" << reg_used << "]" << std::endl;

                    // if we had to push a register, restore it
                    if (reg_pushed) {
                        eval_ss << "\t" << "pop rsi" << std::endl;
                    }
                } else {
                    // automatic memory
                    // get the stack offset; instruction should be something like
                    //      mov rax, [rbp - 4]
                    eval_ss << "\t" << "mov " << reg_string << ", [rbp - " << sym.get_stack_offset() << "]" << std::endl;
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
                    eval_ss << "\t" << "mov rax, [rbp - " << sym.get_stack_offset() << "]" << std::endl;
                } else {
                    /*

                    for automatic memory holding types too big for registers, we want to load the address of where that data lives in the stack
                    so, instead of having something like:
                        mov rax, [rbp - 4]
                    which we would use for something like int or float, we want
                        mov rax, rbp - 4
                    which gives us the address of the data

                    */

                    eval_ss << "\t" << "mov rax, rbp - " << sym.get_stack_offset() << std::endl;
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

    Generates x86 to determine the result of an indexed expression.
    Determines the byte offset based on data type and index, locates the symbol, and fetches the data accordingly

    @param  to_evaluate The indexed expression we are examining
    @param  line    The line number where the expression occurs (for error handling)
    @return A stringstream containing the generated code

    */

    // todo: evaluate indexed expressions
    
    std::stringstream eval_ss;

    // first, get the symbol for the array or string we are indexing
    symbol indexed_sym = *(this->lookup(to_evaluate.getValue(), line).get());

    // ensure it is a valid type to be indexed; if so, get the offset in ebx
    if (
        indexed_sym.get_symbol_type() == VARIABLE && 
        (
            indexed_sym.get_data_type().get_primary() == ARRAY || indexed_sym.get_data_type().get_primary() == STRING)
        ) {
        // todo: perform bounds checking on arrays? the length is known, but we will need to figure out how to handle that error in the assembly, since SIN does not utilize exceptions (could print an error message and quit?)

        // next, get the index and multiply it by the data width to get our byte offset
        eval_ss << this->evaluate_expression(to_evaluate.get_index_value(), line).str();
        // the integer value will now be in eax; multiply by the data width (use MUL for unsigned multiplication -- SIN does not support negative indices)
        eval_ss << "\t" << "mul eax, " << std::dec << indexed_sym.get_data_type().get_width() << std::endl;

        // todo: preserve rbx?

        // move the offset into ebx -- we must use indexing
        eval_ss << "\t" << "mov ebx, eax" << std::endl;
        
        // mark RBX as "in use" in case a future operation requires it
        // this will also cause any function that uses RBX to preserve it when called
        this->reg_stack.peek().set(RBX);
    } else {
        throw TypeException(line);
    }

    // now that we have the index in RBX, fetch the value
    if (indexed_sym.get_data_type().get_qualities().is_const()) {
        // todo: const arrays
        // const arrays can use the name of the variable; their width must be known at compile time
    } else if (indexed_sym.get_data_type().get_qualities().is_dynamic()) {
        // todo: dynamic arrays
        // dynamic arrays will use a pointer -- their width may be variable
    } else if (indexed_sym.get_data_type().get_qualities().is_static()) {
        // todo: static arrays
        // static arrays can use the name of the variable since they're reserved as named variables prior to runtime; their width must be known at compile time
    } else {
        // todo: automatic arrays
        // automatic arrays can live on the stack because their width _must_ be known at compile time
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
    } else {
        // todo: look into compiler table to see if we have a struct -- then make sure this is a "compiler" method
        
        // todo: should a struct name be a valid sizeof argument? we should keep a note in the struct table that says whether its size is known or not
        // todo: maybe structs can't have arrays inside them, only pointers to arrays (or dynamic arrays)
        
        // todo: sizeof<array> and sizeof<string> are invalid
        throw CompilerException("Invalid argument for sizeof<T>; only fixed-width types can be used", compiler_errors::DATA_WIDTH_ERROR, line);
    }

    return eval_ss;
}

std::stringstream compiler::evaluate_unary(Unary &to_evaluate, unsigned int line) {
    /*

    evaluate_unary
    Generates code to evaluate a unary expression

    @param  to_evaluate The unary expression we are evaluating
    @param  line    The line number where the expression occurs
    @return A stringstream containing the generated code

    */

    std::stringstream eval_ss;

    // We need to know the data type in order to evaluate the expression properly
    DataType unary_type = get_expression_data_type(to_evaluate.get_operand(), this->symbol_table, line);

    // first, evaluate the expression we are modifying
    eval_ss << this->evaluate_expression(to_evaluate.get_operand(), line).str();

    // switch to our operator -- only three unary operators are allowed (that don't have special expression types, such as dereferencing or address-of), but only unary minus and unary not have any effect
    switch (to_evaluate.get_operator()) {
        case exp_operator::PLUS:
        {
            // does nothing but is allowed; generates a warning stating as such
			compiler_warning("While the unary plus operator is technically allowed, it does nothing", line);
            break;
        }
        case exp_operator::MINUS:
        {
            // the unary minus operator may only be used on integral and floating-point types
            // this flips the sign on floats and performs two's complement on integers

            if (unary_type.get_primary() == FLOAT) {
                /*

                floating-point types also reverse the sign bit, accomplished through the use of the fchs instruction
                unlike with integers, this will not result in data loss

				the instruction should generate the following code:
					movss/movsd xmm1, [sinl_sp_mask/sinl_dp_mask]
					xorps/xorpd xmm0, xmm1

                */

				// the floating-point expression to negate will already be in the XMM0 register; act based on width
				if (unary_type.get_width() == sin_widths::FLOAT_WIDTH) {
					eval_ss << "movss xmm1, [sinl_sp_mask]" << std::endl;
					eval_ss << "xorps xmm0, xmm1" << std::endl;
				}
				else if (unary_type.get_width() == sin_widths::DOUBLE_WIDTH) {
					eval_ss << "movsd xmm1, [sinl_dp_mask]" << std::endl;
					eval_ss << "xorpd xmm0, xmm1" << std::endl;
				}
				else {
					// todo: width exception? or should 'half' type just be converted to single-precision automatically and generate a warning?
				}
            } else if (unary_type.get_primary() == INT) {
                /*

                integral types will have two's complement performed on them
                if the data is _unsigned_, then it may result in a loss of data because it will not increase the data's width

                */

                if (unary_type.get_qualities().is_unsigned()) {
                    compiler_warning("Note: unary minus on unsigned data may result in a loss of data because the compiler will not increase the data's width", line);
                }

                // the expression is in RAX; check the width to get which register size to use
                std::string register_name;
                if (unary_type.get_width() == sin_widths::SHORT_WIDTH) {
                    register_name = "ax";
                } else if (unary_type.get_width() == sin_widths::INT_WIDTH) {
                    register_name = "eax";
                } else {
                    register_name = "rax";
                }

                // perform two's complement on A with the 'neg' instruction
                eval_ss << "\t" << "neg " << register_name << std::endl;
            } else {
                throw UnaryTypeNotSupportedError(line);
            }

            break;
        }
        case exp_operator::NOT:
        {
            // expression must be a boolean

            if (unary_type.get_primary() == BOOL) {
                // XOR against a bitmask of 0xFF, as a boolean checks for zero or non-zero, not 1 or 0
				// a boolean will be in al
				eval_ss << "\t" << "mov ah, 0xFF" << std::endl;
				eval_ss << "\t" << "xor al, ah" << std::endl;
            } else {
                throw UnaryTypeNotSupportedError(line);
            }

            break;
        }
		case exp_operator::BIT_NOT:
		{
			// expression does not have to be a boolean, it can be any fixed-width type in RAX

			if (unary_type.get_primary() != STRING && unary_type.get_primary() != ARRAY && unary_type.get_primary() != STRUCT) {
				// simply use the x86 NOT instruction
				eval_ss << "\t" << "not rax" << std::endl;
			}
			else {
				throw UnaryTypeNotSupportedError(line);
			}
		}
        default:
            throw IllegalUnaryOperatorError(line);
            break;
    }

    return eval_ss;
}

std::stringstream compiler::evaluate_binary(Binary &to_evaluate, unsigned int line) {
	/*
	
	evaluate_binary
	Generates code for the evaluation of a binary expression

	The binary evaluation algorithm works as follows:
		1. Look at the left operand
			A. Call evaluate_expression on it; this may end up recursively evaluating binary trees
			B. Push necessary register values to the stack to preserve them
		2. Now look at the right operand
			A. Call evaluate_expression on it; again, this may end up recursively evaluating binary trees
			B. Move the result out of RBX or XMM1
			C. Pull the previously pushed values into RAX or XMM0
		3. Generate code for the current expression
			If the value is being returned from a recursive call, the result will be pushed to the stack or moved in registers when necessary

	Note that every time the stack is used, this->stack_offset and max_offset must be adjusted so that the values can be retrieved, pushed, pulled, and stored reliably. If stack_offset and max_offset are not adjusted, errors can and _will_ occur when working with the stack
	
	For example: looking at the expresion 3 * 4 + 5:
		1. Look at the left operand: binary expression 3 * 4
			A. Call evaluate_expression: sends us here
				1. Look at the left hand expression: 3
				2. Look at the right hand expression: 4
				3. Generate code: mul eax, ebx
			B. Push result (rax) to stack
		2. Look at the right operand: literal 5
			A. Evaluate: 5
			B. Move to ebx
			C. Pull value back into eax
		3. Generate code: add eax, ebx

	*/

	std::stringstream eval_ss;

	// get the left and right branches
	DataType left_type = get_expression_data_type(to_evaluate.get_left(), this->symbol_table, line);
	DataType right_type = get_expression_data_type(to_evaluate.get_right(), this->symbol_table, line);

	Type primary = left_type.get_primary();
	size_t data_width = left_type.get_width();
	bool is_signed = left_type.get_qualities().is_signed() || right_type.get_qualities().is_signed();

	// ensure the types are compatible before proceeding with evaluation
	if (left_type.is_compatible(right_type)) {

		// evaluate the left-hand side
		eval_ss << this->evaluate_expression(to_evaluate.get_left(), line).str();
		if (left_type.get_primary() == FLOAT) {
			// todo: preserve xmm0
		}
		else {
			eval_ss << "\t" << "push rax" << std::endl;	// x64 only lets us push 64-bit registers
			// don't need to adjust the compiler's offset adjustment as this will be pulled from the stack before the next statement
		}

		// evaluate the right-hand side
		if (right_type.get_primary() == FLOAT) {
			// this depends on the data width; note that floating-point values must always convert to double if a double is used
			eval_ss << "\t" << ((right_type.get_width() == sin_widths::DOUBLE_WIDTH) ? "movsd" : "movss") << " xmm1, xmm0" << std::endl;

			// todo: restore xmm0 and the stack pointer

			// if the left type is single-precision, but right type is double, we need to convert it to double (if assigning to float, may result in loss of data); this is not considered an 'implicit conversion' by the compiler because both are floating-point types, and requisite width conversions are allowed
			if (left_type.get_width() != right_type.get_width()) {
				if (left_type.get_width() == sin_widths::FLOAT_WIDTH) {
					eval_ss << "\t" << "cvtss2sd xmm0, xmm0" << std::endl;	// todo: is this a valid instruction?
					data_width = sin_widths::DOUBLE_WIDTH;	// ensure the expression is marked as double-precision for eventual operation code generation
				}
				else {
					eval_ss << "\t" << "cvtss2sd xmm1, xmm1" << std::endl;	// convert scalar single to scalar double, taking the value from xmm1 and storing it back in xmm1
				}
			}
		}
		else {
			eval_ss << "\t" << "mov rbx, rax" << std::endl;
			eval_ss << "\t" << "pop rax" << std::endl;
		}

		// finally, act according to the operator and type
		if (to_evaluate.get_operator() == PLUS) {
			switch (primary) {
			case INT:
			case PTR:	// pointer arithmetic with + and - is allowed in SIN with
				eval_ss << "\t" << "add rax, rbx" << std::endl;
				break;
			case FLOAT:
				// single- and double-precision floats use different SSE instructions
				if (data_width == sin_widths::FLOAT_WIDTH) {
					eval_ss << "\t" << "addss xmm0, xmm1" << std::endl;	// add scalar single
				}
				else {
					eval_ss << "\t" << "addsd xmm0, xmm1" << std::endl;	// add scalar double
				}
				break;
			case STRING:
				// todo: string concatenation (passes pointer to string)
				break;
			default:
				// if we have an invalid type, throw an exception
				// todo: should array concatenation be allowed with the + operator?
				throw UndefinedOperatorError("plus", line);
				break;
			}
		}
		else if (to_evaluate.get_operator() == MINUS) {
			switch (primary) {
			case INT:
			case PTR:
				eval_ss << "\t" << "sub rax, rbx" << std::endl;
				break;
			case FLOAT:
				// single- and double-precision floats use different SSE instructions
				if (data_width == sin_widths::FLOAT_WIDTH) {
					eval_ss << "\t" << "subss xmm0, xmm1" << std::endl;
				}
				else {
					eval_ss << "\t" << "subsd xmm0, xmm1" << std::endl;
				}
				break;
			default:
				// the minus operator is undefined for all other types
				throw UndefinedOperatorError("minus", line);
				break;
			}
		}
		else if (to_evaluate.get_operator() == MULT) {
			// mult only allowed for int and float
			if (left_type.get_primary() == INT) {
				// we have to decide between mul and imul instructions -- use imul if either of the operands is signed
				if (is_signed) {
					eval_ss << "\t" << "imul rax, rbx" << std::endl;
				}
				else {
					eval_ss << "\t" << "mul rax, rbx" << std::endl;
				}
			}
			else if (left_type.get_primary() == FLOAT) {
				if (data_width == sin_widths::FLOAT_WIDTH) {
					eval_ss << "\t" << "mulss xmm0, xmm1" << std::endl;
				}
				else {
					eval_ss << "\t" << "mulsd xmm0, xmm1" << std::endl;
				}
			}
			else {
				// todo: throw exception
				throw UndefinedOperatorError("multiplication", line);
			}
		}
		else if (to_evaluate.get_operator() == DIV)
		{
			// div only allowed for int and float
			if (left_type.get_primary() == INT) {
				// how we handle integer division depends on whether we are using signed or unsigned integers
				if (is_signed) {
					// use idiv
					eval_ss << "\t" << "idiv rax, rbx" << std::endl;
				}
				else {
					// use div
					eval_ss << "\t" << "div rax, rbx" << std::endl;
				}
			}
			else if (left_type.get_primary() == FLOAT) {
				// which instruction depends on the width of the values; in either case, we are operating on scalar values (not packed)
				if (data_width == sin_widths::FLOAT_WIDTH) {
					eval_ss << "\t" << "divss xmm0, xmm1" << std::endl;
				}
				else {
					eval_ss << "\t" << "divsd xmm0, xmm1" << std::endl;
				}
			}
			else {
				throw UndefinedOperatorError("division", line);
			}
		}
		else if (to_evaluate.get_operator() == MODULO)
		{
			// modulo only allowed for int and float
			if (left_type.get_primary() == INT) {
				// for modulo, we need to determine what should happen if we are using signed numbers
				// todo: modulo
			}
			else if (left_type.get_primary() == FLOAT) {
				// todo: implement modulo with floating-point numbers
			}
			else {
				throw UndefinedOperatorError("modulo", line);
			}
		}

		// Bitwise operators; these may use int or float
		else if (to_evaluate.get_operator() == exp_operator::BIT_AND)
		{
			if (left_type.get_primary() == INT) {
				// doesn't matter whether we have signed or unsigned data, but we should issue a warning for types of differing widths
				if (left_type.get_width() != right_type.get_width()) {
					compiler_warning("Operands in bitwise operation are different widths", line);
				}

				eval_ss << "\t" << "and rax, rbx" << std::endl;
			}
			else if (left_type.get_primary() == FLOAT) {
				// todo: floats with bitwise operators
			}
			else {
				throw UndefinedOperatorError("bitwise-and", line);
			}
		}
		else if (to_evaluate.get_operator() == exp_operator::BIT_OR)
		{
			// same procedure as bitwise-and
			if (left_type.get_primary() == INT) {
				// doesn't matter whether we have signed or unsigned data, but we should issue a warning for types of differing widths
				if (left_type.get_width() != right_type.get_width()) {
					compiler_warning("Operands in bitwise operation are different widths", line);
				}

				eval_ss << "\t" << "or rax, rbx" << std::endl;
			}
			else if (left_type.get_primary() == FLOAT) {
				// todo: floats with bitwise operators
			}
			else {
				throw UndefinedOperatorError("bitwise-or", line);
			}
		}
		else if (to_evaluate.get_operator() == exp_operator::BIT_XOR)
		{
			// bitwise xor
			if (left_type.get_primary() == INT) {
				// doesn't matter whether we have signed or unsigned data, but we should issue a warning for types of differing widths
				if (left_type.get_width() != right_type.get_width()) {
					compiler_warning("Operands in bitwise operation are different widths", line);
				}

				eval_ss << "\t" << "xor rax, rbx" << std::endl;
			}
			else if (left_type.get_primary() == FLOAT) {
				// todo: floats with bitwise operators
			}
			else {
				throw UndefinedOperatorError("bitwise-xor", line);
			}
		}
		// bitwise not is a unary operator

		// Logical operators; these may only use boolean types
		else if (to_evaluate.get_operator() == exp_operator::AND)
		{
			// logical and
		}
		else if (to_evaluate.get_operator() == exp_operator::OR)
		{
			// logical or
		}
		else if (to_evaluate.get_operator() == exp_operator::XOR)
		{
			// logical xor
		}
		// logical not is a unary operator

		// since we have tested all other valid operators, it must be an equivalency operator
		else {
			/*

			Equivalency operators may be used on all types
			
			The equivalency operators will use the CMP instruction on RAX and RBX to test for (in)equality, then use the SETcc instruction to set RAX
			This will result in 1 in AL if the condition was true, or 0 if it was false; we then use MOVZX to extend AL to RAX, ensuring we don't have any garbage data remaining in higher bits in the register

			*/

			// write the comparison
			eval_ss << "\t" << "cmp rax, rbx" << std::endl;

			// a variable to hold our instruction mnemonic
			std::string instruction = "";

			// todo: we could write a simple utility function to get a string for the equality based on an operator (e.g., turning EQUAL into 'e' or LESS OR EQUAL to 'le'), assuming we need to use it more than once

			// now, switch to determine which branching instruction we need
			switch (to_evaluate.get_operator()) {
			case exp_operator::EQUAL:
				instruction = "sete";
				break;
			case exp_operator::NOT_EQUAL:
				instruction = "setne";
				break;
			case exp_operator::GREATER:
				instruction = "setg";
				break;
			case exp_operator::LESS:
				instruction = "setl";
				break;
			case exp_operator::GREATER_OR_EQUAL:
				instruction = "setge";
				break;
			case exp_operator::LESS_OR_EQUAL:
				instruction = "setle";
				break;
			default:
				// if the parser didn't catch a 'no operator', throw the exception here -- we have no more valid operators
				throw CompilerException("Undefined operator", compiler_errors::UNDEFINED_ERROR, line);
				break;
			}

			// write the instruction sequence
			eval_ss << "\t" << instruction << " al" << std::endl;
			eval_ss << "\t" << "movzx rax, al" << std::endl;
		}
	}
	else {
		// if the types were not compatible, throw a type error
		throw TypeException(line);
	}

	// finally, return the generated code
	return eval_ss;
}
