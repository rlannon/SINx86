/*

SIN Toolchain (x86 target)
evaluate_expression.cpp

Generates code for evaluating an expression. This data will be loaded into registers as specified by "Doc/Registers"

*/

#include "compiler.h"

// todo: add evaluation of expressions labeled 'constexpr'

// todo: create an expression evaluation class and give it access to compiler members?
std::pair<std::string, size_t> compiler::evaluate_expression(
    std::shared_ptr<Expression> to_evaluate,
    unsigned int line,
    DataType *type_hint
) {
    /*

    evaluate_expression
    Generates code to evaluate a given Expression object

    This function will determine how many such objects must be freed in each expression so they can be properly handled by their parent expression. For example:
        @print("Found " + @itos(10) + " primes!\n");
    We need to utilize a string returned by the function `itos`, but this string must have an RC of 1 when returned.
    After the call to print, this data becomes unreachable (as it will be copied into a new area of memory, the variable `s`).
    As such, when we call `print`:
        * The binary expression `"Found " + @itos(10)` is evaluated
        * The delegate function to evaluate `itos` will see that it has something to free, returning <string, 1>; the address of this data will be preserved on the stack
        * Once the binary expression is crafted, the count will be reduced, the address popped from the stack, and the data freed
        * The next concatenation with `<string buffer> + " primes!\n"` will be crafted, and since there is no temporary data to free, it will produce code as normal

    */

    // todo: proper type hints -- e.g., if we are assigning like `alloc long int a: 1_000`, then we should ensure it treats the literal as a `long`, not regular, `int`
    // although literal 1000 could fit in a long int, we need to ensure that the sign is extended properly
    // this could be done in the assignment tool, automatically sign-extending 32-bit integers to 64-bit

    std::stringstream evaluation_ss;
    size_t count = 0;

    // The expression evaluation depends on the expression's type
    switch (to_evaluate->get_expression_type()) {
        case LITERAL:
        {
            // get the literal
            Literal literal_exp = *dynamic_cast<Literal*>(to_evaluate.get());

            // dispatch to our evaluation function
            evaluation_ss = this->evaluate_literal(literal_exp, line, type_hint);
            break;
        }
        case IDENTIFIER:
        {
            // get the lvalue
            Identifier lvalue_exp = *dynamic_cast<Identifier*>(to_evaluate.get());

            // dispatch to our evaluation function
            evaluation_ss = this->evaluate_identifier(lvalue_exp, line);
            break;
        }
        case INDEXED:
        {
            // get the address and dereference
            DataType t = expression_util::get_expression_data_type(to_evaluate, this->symbols, this->structs, line);
            evaluation_ss << this->get_exp_address(to_evaluate, RBX, line).str();
            evaluation_ss << "\t" << "mov " << register_usage::get_register_name(RAX, t) << ", [rbx]" << std::endl;
            break;
        }
        case LIST:
        {
            // evaluate a list expression

            // todo: would it be better to just iterate on an assignment and copy into the list? or is it better to use array_copy (as we are doing now)?
            
            // create our label
            std::string list_label = compiler::LIST_LITERAL_LABEL + std::to_string(this->list_literal_num);
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
            DataType t = expression_util::get_expression_data_type(to_evaluate, this->symbols, this->structs, line);
            auto le = dynamic_cast<ListExpression*>(to_evaluate.get());
            t.set_primary(le->get_list_type());

            size_t width = expression_util::get_width(
                t,
                this->evaluator,
                this->structs,
                this->symbols,
                this->current_scope_name,
                this->current_scope_level,
                line
            );  // todo: fix this
           
            /*

            here, t supposedly has a width of 4GB -- it should be 4 bytes; figure out
                * why it isn't updating the length here
                * why it thinks the length is 4gb in both cases

            */

            size_t offset = 0;

            // get the address in R15
            evaluation_ss << "\t" << "lea r15, [" << list_label << "]" << std::endl;
            if (t.get_primary() == ARRAY) {
                // write in the length if we have an array
                evaluation_ss << "\t" << "mov eax, " << le->get_list().size() << std::endl;
                evaluation_ss << "\t" << "mov [r15], eax" << std::endl;

                // increment the pointer by one dword
                evaluation_ss << "\t" << "add r15, " << sin_widths::INT_WIDTH << std::endl;
            }

            // now, iterate
            for (size_t i = 0; i < le->get_list().size(); i++) {
                // first, make sure the type of this expression matches the list's subtype
                auto m = le->get_list().at(i);
                DataType member_type = expression_util::get_expression_data_type(m, this->symbols, this->structs, line);
                
                // todo: support lists of strings and arrays (utilize references and copies) -- could utilize RBX for this
                if (t.get_primary() == ARRAY && member_type != t.get_subtype()) {
                    throw CompilerException(
                        "Type mismatch (arrays must be homogeneous)",
                        compiler_errors::TYPE_ERROR,
                        line
                    );
                }
                else if (t.get_primary() == TUPLE && (t.get_contained_types().at(i) != member_type)) {
                    throw CompilerException(
                        "Tuple type mismatch",
                        compiler_errors::TYPE_ERROR,
                        line
                    );
                }
                
                // adjust the type hint
                DataType hinted_type;
                DataType *to_pass = nullptr;
                try {
                    hinted_type = type_hint->get_contained_types().at(i);
                    to_pass = &hinted_type;
                } catch (std::exception &e) {
                    to_pass = nullptr;
                }

                auto member_p = this->evaluate_expression(m, line, to_pass);
                evaluation_ss << member_p.first;
                count += member_p.second;

                // store it in [r15 + offset]
                if (member_type.get_primary() == FLOAT) {
                    std::string inst = member_type.get_width() == sin_widths::DOUBLE_WIDTH ? "movsd" : "movss";
                    evaluation_ss << "\t" << inst << " [r15 + " << offset << "], xmm0" << std::endl;
                }
                else {
                    std::string reg_name = register_usage::get_register_name(RAX, member_type);
                    evaluation_ss << "\t" << "mov [r15 + " << offset << "], " << reg_name << std::endl;
                }
                
                // update the offset within the list of the element to which we are writing
                offset += member_type.get_width();
            }

            // move the list address into RAX
            evaluation_ss << "\t" << "lea rax, [" << list_label << "]" << std::endl;

            // restore R15 and RCX, if we pushed them
            if (pushed_r15) {
                evaluation_ss << "\t" << "pop r15" << std::endl;
            }

            // todo: adapt this to properly handle tuple literals

            if (t.get_primary() == ARRAY) {
                // finally, utilize the .bss section and the 'res' directive for our list
                std::string res_instruction;
                size_t subtype_width = t.get_subtype().get_width();
                if (subtype_width == 8) {
                    res_instruction = "resq";
                }
                else if (subtype_width == 4) {
                    res_instruction = "resd";
                }
                else if (subtype_width == 2) {
                    res_instruction = "resw";
                }
                else {
                    res_instruction = "resb";
                }
                this->bss_segment << list_label << ": resd 1" << std::endl; 
                this->bss_segment << list_label << "_data: " << res_instruction << " " << le->get_list().size() << std::endl;
            }
            else {
                this->bss_segment << list_label << ": resb " << width << std::endl;
            }
            
            break;
        }
        case BINARY:
        {
			// cast to Binary class and dispatch
			Binary bin_exp = *dynamic_cast<Binary*>(to_evaluate.get());
            auto bin_p = this->evaluate_binary(bin_exp, line, type_hint);
            evaluation_ss << bin_p.first;
            count += bin_p.second;

            // todo: clean up binary operands here?

			break;
        }
        case UNARY:
        {
			Unary unary_exp = *dynamic_cast<Unary*>(to_evaluate.get());
			evaluation_ss << this->evaluate_unary(unary_exp, line, type_hint).str();
            // todo: clean up unary?
            break;
        }
        case VALUE_RETURNING_CALL:
        {
            ValueReturningFunctionCall call_exp = *dynamic_cast<ValueReturningFunctionCall*>(to_evaluate.get());
            auto call_p = this->call_function(call_exp, line, false);  // don't allow void functions here
            
            // add the call code
            evaluation_ss << call_p.first;

            // add the count from this expression to our current count
            // any parameters that returned a reference (but passed by value) were already dealt with
            count += call_p.second;
            if (call_p.second) {
                evaluation_ss << "; RAX now contains value to clean up" << std::endl;
                evaluation_ss << "\t" << "push rax" << "\t" << "; we must push so we can free later" << std::endl;
                // todo: fix this, it's really dumb how this works right now
            }
            break;
        }
        case CAST:
        {
            auto c = dynamic_cast<Cast*>(to_evaluate.get());

            // ensure the type to which we are casting is valid
            if (DataType::is_valid_type(c->get_new_type())) {
                // check to make sure the typecast itself is valid (follows the rules)
                DataType old_type = expression_util::get_expression_data_type(c->get_exp(), this->symbols, this->structs, line);
                if (is_valid_cast(old_type, c->get_new_type())) {
                    // if we are casting a literal integer or float to itself (but with a different width), create a new Literal
                    if (
                        (c->get_exp()->get_expression_type() == LITERAL) &&
                        (old_type.get_primary() == c->get_new_type().get_primary()) &&
                        (old_type.get_primary() == INT || old_type.get_primary() == FLOAT)
                    ) {
                        // update the type
                        std::shared_ptr<Literal> contained = std::dynamic_pointer_cast<Literal>(c->get_exp());
                        contained->set_type(c->get_new_type());

                        // now, evaluate
                        evaluation_ss << this->evaluate_expression(contained, line, type_hint).first;
                    }
                    else {
                        // to perform the typecast, we must first evaluate the expression to be casted
                        auto cast_p = this->evaluate_expression(c->get_exp(), line, type_hint);
                        evaluation_ss << cast_p.first;

                        // now, use the utility function to actually cast the type
                        evaluation_ss << cast(old_type, c->get_new_type(), line).str();
                    }
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
            auto t = expression_util::get_expression_data_type(attr->get_selected(), this->symbols, this->structs, line);
            auto attr_p = this->evaluate_expression(attr->get_selected(), line, type_hint);
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
                    evaluation_ss << attr_p.first;
                    count += attr_p.second;

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

                Note this can return the sizes of const- and variable-width types alike
                
                */

                if (t.get_primary() == STRUCT) {
                    auto s = this->get_struct_info(t.get_struct_name(), line);
                    evaluation_ss << "\t" << "mov eax, " << s.get_width() << std::endl;
                }
                else if (t.get_primary() == ARRAY || t.get_primary() == STRING) {
                    evaluation_ss << attr_p.first;
                    count += attr_p.second;
                    evaluation_ss << "\t" << "mov eax, [rax]" << std::endl;

                    // strings have a type width of 1, if it's an array we need to get the width
                    size_t type_width = 1;
                    if (t.get_primary() == ARRAY) {
                        type_width = t.get_subtype().get_width();
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

    // if we have a count greater than 1, we can free a few references
    if (count > 1) {
        evaluation_ss << "; Have more than 1 reference to free" << std::endl;
        evaluation_ss << "\t" << "pop r12" << std::endl;
        evaluation_ss << "\t" << "mov r13, rax" << std::endl;
        for (size_t i = 1; i < count; i++) {
            evaluation_ss << "\t" << "pop rdi" << std::endl;
            evaluation_ss << call_sre_function("_sre_free");
        }
        evaluation_ss << "\t" << "push r12" << std::endl;
        evaluation_ss << "\t" << "mov rax, r13" << std::endl;

        // now, we can set the count to 1 because we handled the references that could be dealt with
        count = 1;
    }

    // return the pair
    return std::make_pair<>(evaluation_ss.str(), count);
}

std::stringstream compiler::evaluate_literal(Literal &to_evaluate, unsigned int line, DataType *type_hint) {
    /*

    evaluate_literal
    Evaluates a literal expression

    This function will generate code to put a literal value in the A register, or on the stack if necessary. Note that the value will be in RAX, EAX, AX, or AL depending on the data width.
    This function will add data to the compiler's .data or .rodata section where needed.
	Note that pointer literals are considered 'address-of' expressions and so are not included in this function.

    @param  to_evaluate The literal expression we are evaluating
    @param  line    The line number of the expression (for error handling)
    @param  type_hint   A hint about the proper data type for the literal expression
    @return A stringstream containing the generated code

    */

    std::stringstream eval_ss;

    // act based on data type and width -- use type_hint if we have one
    // todo: verify that the type hint is appropriate?
    DataType type;
    if (type_hint) {
        type = *type_hint;
    }
    else {
        type = to_evaluate.get_data_type();
    }

    if (type.get_primary() == VOID) {
        // A void literal gets loaded into rax as 0
        // These are used in return statements for void-returning functions
        eval_ss << "\t" << "mov rax, 0" << std::endl;
    } 
    else if (type.get_primary() == INT) {
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
    } 
    else if (type.get_primary() == FLOAT) {
        /*

        Floats cannot be used as immediate values; as such, we will need to define them in the .data section and load XMM0
        If there is no width specifier used on the literal, we will default to float (not double)

        */

        std::string float_label = compiler::FLOAT_LITERAL_LABEL + std::to_string(this->fltc_num);
        this->fltc_num += 1;

        // todo: check to see if the width was specified with a type suffix

        // if we have a single-precision
        std::string res_directive = "dd";
        std::string inst = "movss";

        // todo: double-precision

        data_segment << float_label << ": " << res_directive << " " << to_evaluate.get_value() << std::endl;
        eval_ss << "\t" << inst << " xmm0, [" << float_label << "]" << std::endl;
    } 
    else if (type.get_primary() == BOOL) {
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
    } 
    else if (type.get_primary() == CHAR) {
        // Since SIN uses ASCII (for now), all chars get loaded into al as they are only a byte wide
        if (type.get_width() == sin_widths::CHAR_WIDTH) {
            // NASM supports an argument like 'a' for mov to load the char's ASCII value
            // note: use backticks because we want to use escape characters
            eval_ss << "\t" << "mov al, `" << to_evaluate.get_value() << "`" << std::endl;
        } else {
            throw CompilerException("Unicode currently not supported", compiler_errors::UNICODE_ERROR, line);
        }
    }
    else if (type.get_primary() == STRING) {
        /*
        
        Keep in mind that strings are really _pointers_ under the hood -- so we will return a pointer to the string literal, not the string itself on the stack

        However, we need to reserve space for the string in the .rodata segment

        */

        std::string name = compiler::CONST_STRING_LABEL + std::to_string(this->strc_num);

        // actually reserve the data and enclose the string in backticks in case we have escaped characters
        this->rodata_segment << "\t" << name << "\t" << "dd " << to_evaluate.get_value().length() << ", `" << to_evaluate.get_value() << "`, 0" << std::endl;
        this->strc_num += 1;

        // now, load the a register with the address of the string
        eval_ss << "\t" << "lea rax, [" << name << "]" << std::endl;
    }
    else {
        // invalid data type
        throw TypeException(line);	// todo: enable JSON-style objects to allow struct literals?
    }

    // finally, return the generated code
    return eval_ss;
}

std::stringstream compiler::evaluate_identifier(Identifier &to_evaluate, unsigned int line) {
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
        compiler_warning(
            "Symbol '" + sym.get_name() + "' may have been freed",
            compiler_errors::DATA_FREED,
            line
        );

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
            }
            else if (can_pass_in_register(sym.get_data_type())) {
                // the data width determines which register size to use
                std::string reg_string = get_rax_name_variant(sym.get_data_type(), line);

                // how we get this data depends on where it lives
                if (sym.get_data_type().get_qualities().is_static()) {
                    // static memory can be looked up by name -- variables are in the .bss, .data, or .rodata section
                    eval_ss << "\t" << "lea rax, [" << sym.get_name() << "]" << std::endl;
                    eval_ss << "\t" << "mov " << reg_string << ", [rax]" << std::endl;
                } 
                else if (sym.get_data_type().get_qualities().is_dynamic()) {
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
                        // if it contains a symbol, store it back in the stack and mark it as not in a register
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
                    } 
                    else {
                        reg_used = register_usage::get_register_name(r);
                    }

                    // get the dereferenced pointer in A
                    eval_ss << "\t" << "mov " << reg_used << ", [rbp - " << sym.get_offset() << "]" << std::endl;
                    eval_ss << "\t" << "mov " << reg_string << ", [" << reg_used << "]" << std::endl;

                    // if we had to push a register, restore it
                    if (reg_pushed) {
                        eval_ss << "\t" << "pop rsi" << std::endl;
                    }
                } 
                else {
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
            } 
            else {
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
        } 
        else {
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

    The only valid types for indexing are arrays and strings; tuples are to be accessed like structs, with the dot operator.

    @param  to_evaluate The indexed expression we are examining
    @param  line    The line number where the expression occurs (for error handling)
    @return A stringstream containing the generated code

    */
    
    std::stringstream eval_ss;

    DataType to_index_type = expression_util::get_expression_data_type(to_evaluate.get_to_index(), this->symbols, this->structs, line);
    if (is_subscriptable(to_index_type.get_primary())) {
        // todo: evaluate indexed
        eval_ss << "\t" << "; todo: subscripting" << std::endl;
    }
    else {
        throw TypeNotSubscriptableException(line);
    }

    // return our generated code
    return eval_ss;
}
