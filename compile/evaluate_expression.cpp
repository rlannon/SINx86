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

        if (type.get_width() == sin_widths::CHAR_WIDTH) {
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
    
    std::stringstream eval_ss;

    // first, get the symbol for the array or string we are indexing
    symbol indexed_sym = *(this->lookup(to_evaluate.getValue(), line).get());

    // ensure it is a valid type to be indexed; if so, get the offset in ebx
    if (indexed_sym.get_symbol_type() == VARIABLE && 
		(
			indexed_sym.get_data_type().get_primary() == ARRAY || 
			indexed_sym.get_data_type().get_primary() == STRING)
		) {
        // todo: perform bounds checking on arrays? the length is known, but we will need to figure out how to handle that error in the assembly, since SIN does not utilize exceptions (could print an error message and quit?)

        // next, get the index and multiply it by the data width to get our byte offset
        eval_ss << this->evaluate_expression(to_evaluate.get_index_value(), line).str();
		
		// preserve the element number in edx
		eval_ss << "\t" << "mov edx, eax" << std::endl;

        // the integer value will now be in eax; multiply by the data width (use MUL for unsigned multiplication -- SIN does not support negative indices)
        eval_ss << "\t" << "mul eax, " << std::dec << indexed_sym.get_data_type().get_width() << std::endl;

        // move the offset into ebx and add INT_WIDTH to it so that we can account for the width of the array
        eval_ss << "\t" << "mov ebx, eax" << std::endl;
		eval_ss << "\t" << "add ebx, " << sin_widths::INT_WIDTH << std::endl;
        
        // mark RBX as "in use" in case a future operation requires it
        // this will also cause any function that uses RBX to preserve it when called
        this->reg_stack.peek().set(RBX);
		this->reg_stack.peek().set(RCX);	// todo: use RSI for the source?
		this->reg_stack.peek().set(RDX);
    } else {
        throw TypeException(line);
    }

	// get the register name so that we can ensure we are reading a value of the appropriate width
	std::string reg_name = get_rax_name_variant(indexed_sym.get_data_type(), line);

    // now that we have the offset in RBX, fetch the value
	if (indexed_sym.get_data_type().get_qualities().is_static()) {
		/*
		
		Static arrays can use the name of the variable since they're reserved as named variables prior to runtime
		Note that because they are static, their width must be known at compile time

		The first element in the array is the lowest in memory, and subsequent elements are [array_base + offset]
		
		*/

		eval_ss << "\t" << "mov eax, [" << indexed_sym.get_name() << "]" << std::endl;
		eval_ss << "\t" << "cmp edx, eax" << std::endl;	// perform the bounds check
		// todo: SIN runtime errors; handle the error if necessary

		// access the element
		eval_ss << "\t" << "mov " << reg_name << ", [" << indexed_sym.get_name() << " + rbx]" << std::endl;
	} 
	else if (indexed_sym.get_data_type().get_qualities().is_dynamic()) {
        /*
		
		Dynamic arrays will use a pointer; their width may be variable
		Like static arrays, the first element of the array is at the lowest area in memory, and each subsequent element is at a higher address

		*/

		eval_ss << "\t" << "mov rcx, [rbp - " << indexed_sym.get_stack_offset() << "]" << std::endl;

		// perform a bounds check
		eval_ss << "\t" << "mov eax, [rcx]" << std::endl;	// get the length of the array
		eval_ss << "\t" << "cmp edx, eax" << std::endl;	// compare EDX (element's index number) with EAX (number of elements in the array)
		// todo: SIN runtime errors -- run an error routine if we are out of bounds
		
		eval_ss << "\t" << "add rcx, rbx" << std::endl;	// add the element's offset (ebx) to rcx
		eval_ss << "\t" << "mov " << reg_name << ", [rcx]" << std::endl;	// dereference the pointer
    }
	else {
        /*
		
		Automatic arrays can live on the stack because their width _must_ be known at compile time
		Following the convention of other stack-allocated variables, the first element of the array lives in the highest memory location, and subsequent elements are accessed by [stack_base - array_offset - element_offset]

		The algorithm is simple:
			- Get the address of the base element for the array
			- Subtract the element's index from that pointer
			- Dereference the pointer

		*/

		eval_ss << "\t" << "mov rcx, rbp" << std::endl;
		eval_ss << "\t" << "sub rcx, " << indexed_sym.get_stack_offset() << std::endl;

		// perform a bounds check
		eval_ss << "\t" << "mov eax, [rcx]" << std::endl;
		eval_ss << "\t" << "cmp edx, eax" << std::endl;	// compare EDX (element's index number) with EAX (number of elements in the array)
		// todo: SIN runtime errors, jump to error routine if the element access is out of bounds

		eval_ss << "\t" << "sub rcx, rbx" << std::endl;
		eval_ss << "\t" << "mov " << reg_name << ", [rcx]" << std::endl;
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
