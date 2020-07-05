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

    // todo: allow a 'reg' object to be supplied instead of giving all values in RAX

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
            // get the address and dereference
            DataType t = get_expression_data_type(to_evaluate, this->symbols, this->structs, line);
            evaluation_ss << this->get_exp_address(to_evaluate, RBX, line).str();
            evaluation_ss << "\t" << "mov " << register_usage::get_register_name(RAX, t) << ", [rbx]" << std::endl;
            break;
        }
        case LIST:
        {
            // evaluate a list expression

            // todo: would it be better to just iterate on an assignment and copy into the list? or is it better to use array_copy (as we are doing now)?
            
            // create our label
            std::string list_label = "sinl_list_literal_" + std::to_string(this->list_literal_num);
            this->list_literal_num += 1;

            // preserve R15, as it will be used to keep track of where our list is
            bool pushed_r15 = false;
            if (this->reg_stack.peek().is_in_use(R15)) {
                symbol *contained = this->reg_stack.peek().get_contained_symbol(R15);
                if (contained) {
                    contained->set_register(NO_REGISTER);
                    this->reg_stack.peek().clear(R15);
                }
                else {
                    evaluation_ss << "\t" << "push r15" << std::endl;
                    pushed_r15 = true;
                }
            }

            // get our type information
            DataType t = get_expression_data_type(to_evaluate, this->symbols, this->structs, line);
            auto le = dynamic_cast<ListExpression*>(to_evaluate.get());
            size_t offset = 0;
            
            // get the address in RDI
            evaluation_ss << "\t" << "lea rdi, [" << list_label << "]" << std::endl;

            // now, iterate
            for (auto m: le->get_list()) {
                // first, make sure the type of this expression matches the list's subtype
                DataType member_type = get_expression_data_type(m, this->symbols, this->structs, line);
                
                // todo: support lists of strings and arrays (utilize references and copies) -- could utilize RBX for this
                
                if (member_type == *t.get_full_subtype()) {
                    // evaluate the expression
                    evaluation_ss << this->evaluate_expression(m, line).str();

                    // store it in [r15 + offset]
                    std::string rax_name = register_usage::get_register_name(RAX, member_type);
                    evaluation_ss << "mov [r15 + " << offset << "], " << rax_name << std::endl;

                    // update the offset within the list of the element to which we are writing
                    offset += member_type.get_width();
                }
                else {
                    throw CompilerException(
                        "Type mismatch (lists must be homogeneous)",
                        compiler_errors::TYPE_ERROR,
                        line
                    );
                }
            }

            // restore R15 and RCX, if we pushed them
            if (pushed_r15) {
                evaluation_ss << "\t" << "pop r15" << std::endl;
            }

            // finally, utilize the .bss section and the 'res' directive for our list
            std::string res_instruction;
            if (t.get_width() == 8) {
                res_instruction = "resq";
            }
            else if (t.get_width() == 4) {
                res_instruction = "resd";
            }
            else if (t.get_width() == 2) {
                res_instruction = "resw";
            }
            else {
                res_instruction = "resb";
            }
            this->bss_segment << list_label << ": " << res_instruction << " " << le->get_list().size() << std::endl;
            
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
				evaluation_ss << m.evaluate(this->symbols, this->structs, line).str();

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
        case CAST:
        {
            auto c = dynamic_cast<Cast*>(to_evaluate.get());

            // ensure the type to which we are casting is valid
            if (DataType::is_valid_type(c->get_new_type())) {
                // check to make sure the typecast itself is valid (follows the rules)
                DataType old_type = get_expression_data_type(c->get_exp(), this->symbols, this->structs, line);
                if (is_valid_cast(old_type, c->get_new_type())) {
                    // to perform the typecast, we must first evaluate the expression to be casted
                    evaluation_ss << this->evaluate_expression(c->get_exp(), line).str();

                    // now, use the utility function to actually cast the type
                    evaluation_ss << cast(old_type, c->get_new_type(), line).str();
                }
                else {
                    throw InvalidTypecastException(line);
                }
            }
            else {
                throw TypeException(line);
            }
            break;
        }
        case ATTRIBUTE:
        {
            auto attr = dynamic_cast<AttributeSelection*>(to_evaluate.get());
            auto t = attr->get_data_type();

            // we have a limited number of attributes
            if (attr->get_attribute() == LENGTH) {
                /*

                length attribute
                Returns the number of elements in a collection.
                
                For string and array types, it is the first doubleword at the data
                For struct types, it is the number of fields -- known at compile-time

                For all other types, returns 1

                */

                if (t.get_primary() == ARRAY || t.get_primary() == STRING) {
                    // First, evaluate the selected expression
                    evaluation_ss << this->evaluate_expression(attr->get_selected(), line).str();
                    evaluation_ss << "\t" << "mov eax, [rax]" << std::endl;
                }
                else if (t.get_primary() == STRUCT) {
                    auto s = this->get_struct_info(t.get_struct_name(), line);
                    evaluation_ss << "\t" << "mov eax, 1" << std::endl; // todo: we need a get_fields method
                }
                else {
                    evaluation_ss << "\t" << "mov eax, 1" << std::endl;
                }
            }
            else if (attr->get_attribute() == SIZE) {
                /*
                
                size attribute
                Returns the number of bytes occupied by the data.

                Roughly equivalent to sizeof< T >, except it can return the sizes of variable-width types
                
                */

                if (t.get_primary() == STRUCT) {
                    auto s = this->get_struct_info(t.get_struct_name(), line);
                    evaluation_ss << "\t" << "mov eax, " << s.get_width() << std::endl;
                }
                else if (t.get_primary() == ARRAY || t.get_primary() == STRING) {
                    evaluation_ss << this->evaluate_expression(attr->get_selected(), line).str();
                    evaluation_ss << "\t" << "mov eax, [rax]" << std::endl;

                    // strings have a type width of 1, if it's an array we need to get the width
                    size_t type_width = 1;
                    if (t.get_primary() == ARRAY) {
                        type_width = t.get_full_subtype()->get_width();
                    }

                    evaluation_ss << "\t" << "mov rbx, " << type_width << std::endl;
                    evaluation_ss << "\t" << "mul rbx" << std::endl;
                }
                else {
                    evaluation_ss << "\t" << "mov eax, " << t.get_width() << std::endl;
                }
            }
            else if (attr->get_attribute() == VARIABILITY) {
                // todo: variability
                throw CompilerException("Not yet implemented", compiler_errors::UNKNOWN_ATTRIBUTE, line);
            }
            else {
                throw CompilerException(
                    "Invalid attribute",
                    compiler_errors::UNKNOWN_ATTRIBUTE,
                    line
                );
            }

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
            // note that we want the unused high bytes to be zero in case this value gets stored at a 32-bit location
            // we can't just load the 32-bit register though, as this would mess with signed values
            eval_ss << "\t" << "mov ax, " << to_evaluate.get_value() << std::endl;
            eval_ss << "\t" << "movzx eax, ax" << std::endl;    // so we use movzx to accomplish this
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
        this->rodata_segment << "\t" << name << "\t" << "dd " << to_evaluate.get_value().length() << ", `" << to_evaluate.get_value() << "`, 0" << std::endl;
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
        compiler_warning("Symbol '" + sym.get_name() + "' may have been freed", compiler_errors::DATA_FREED, line);

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

                // how we get this data depends on where it lives
                if (sym.get_data_type().get_qualities().is_static()) {
                    // static memory can be looked up by name -- variables are in the .bss, .data, or .rodata section
                    eval_ss << "\t" << "lea rax, [" << sym.get_name() << "]" << std::endl;
                    eval_ss << "\t" << "mov " << reg_string << ", rax" << std::endl;
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
                        // since no register is available, use rsi
                        // if it contains a symbol, just 
                        symbol *contained = this->reg_stack.peek().get_contained_symbol(RSI);
                        reg_used = "rsi";
                        if (contained == nullptr) {
                            eval_ss << "\t" << "push rsi" << std::endl;
                            reg_pushed = true;
                        }
                        else {
                            eval_ss << "\t" << "mov [rbp - " << contained->get_offset() << "], " << register_usage::get_register_name(RSI, contained->get_data_type()) << std::endl;
                            contained->set_register(NO_REGISTER);
                            this->reg_stack.peek().clear_contained_symbol(RSI);
                        }
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
                    /*
                    
                    automatic memory
                    get the stack offset; instruction should be something like
                        mov rax, [rbp - 4]
                    If the value is in a register, then just perform a register move
                    
                    */

                    if (sym.get_register() == NO_REGISTER) {
                        eval_ss << "\t" << "mov " << reg_string << ", [rbp - " << sym.get_offset() << "]" << std::endl;
                    }
                    else {
                        eval_ss << "\t" << "mov rax, " << register_usage::get_register_name(sym.get_register()) << std::endl;
                    }
                }

                return eval_ss;
            } else {
                // values too large for registers will use _pointers_, although the SIN syntax hides this fact
                // therefore, all data will go in rax
                if (sym.get_data_type().get_qualities().is_static()) {
                    // static pointers
                    eval_ss << "\t" << "lea rax, [" << sym.get_name() << "]" << std::endl;
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
                        mov rax, rbp
                        sub rax, 4
                    which gives us the address of the data

                    */

                    if (sym.get_data_type().get_primary() == STRING) {
                        eval_ss << "\t" << "mov rax, [rbp - " << sym.get_offset() << "]" << std::endl;
                    } else {
                        if (sym.get_offset() < 0) {
                            eval_ss << "\t" << "lea rax, [rbp + " << -sym.get_offset() << "]" << std::endl;
                        }
                        else {
                            eval_ss << "\t" << "lea rax, [rbp - " << sym.get_offset() << "]" << std::endl;
                        }
                    }
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
