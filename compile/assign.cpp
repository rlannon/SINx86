/*

SIN Toolchain (x86 target)
assign.cpp

Implementation of the assignment functions for the compiler

*/

#include "compiler.h"

std::stringstream compiler::assign(Assignment assign_stmt) {
    /*

    assign
    Generates code for an assignment statement

    This function dispatches the work for making assignments to the appropriate handlers.
	It will also ensure that we are not assigning to const data or to already-initialized final data.

    @param  assign_stmt The Assignment object that contains the information we need to make the assignment
    @return A stringstream containing the generated code

    */

    // find the symbol
    LValue *lvalue = dynamic_cast<LValue*>(assign_stmt.get_lvalue().get());
    symbol &sym = *(this->lookup(lvalue->getValue(), assign_stmt.get_line_number()).get()); // will throw an exception if the symbol doesn't exist

    // ensure we can make the assignment
    if (sym.get_data_type().get_qualities().is_const()) {
        // ensure we aren't assigning to a const-qualified variable
        throw ConstAssignmentException(assign_stmt.get_line_number());
	}
	else if (sym.get_data_type().get_qualities().is_final() && sym.was_initialized()) {
		// ensure we don't write to final data if it has been initialized
		throw FinalAssignmentException(assign_stmt.get_line_number());
	} else if (sym.get_symbol_type() == FUNCTION_SYMBOL) {
        // if the symbol is a function symbol, then we have an error
        throw InvalidSymbolException(assign_stmt.get_line_number());
    } else {
        // dispatch to the assignment handler
        return this->handle_assignment(sym, assign_stmt.get_rvalue(), assign_stmt.get_line_number());
    }
}

std::stringstream compiler::handle_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line) {
    /*

    handle_assignment
    Generates code to assign 'value' to 'sym'

    We need this as a separate function because it will be called by the allocation code generator if the user uses alloc-assign syntax
	As a result, this function will *not* check for initialized const/final data members; that check is done by compiler::assign

    @param  sym The symbol to which we are assigning data
    @param  value   A shared pointer to the expression for the assignment
    @return A stringstream containing the generated code

    */

	std::stringstream handle_ss;

    // First, we need to determine some information about the symbol
    // Look at its type and dispatch accordingly
    DataType symbol_type = sym.get_data_type();
    DataType expression_type = get_expression_data_type(value, this->symbol_table, line);

    // ensure our expression's data type is compatible with our variable's data type
    if (symbol_type.is_compatible(expression_type)) {
        // dispatch appropriately based on the data type
        switch(symbol_type.get_primary()) {
            case INT:
                handle_ss = handle_int_assignment(sym, value, line);
				break;
            case CHAR:
                break;
            case FLOAT:
                break;
            case BOOL:
				handle_ss = handle_bool_assignment(sym, value, line);
				break;
            case PTR:
			{
				// check that the type qualities are valid - pointers have special rules due to the language's type variability policy
				std::shared_ptr<DataType> left_type = sym.get_data_type().get_full_subtype();
				std::shared_ptr<DataType> right_type = get_expression_data_type(value, this->symbol_table, line).get_full_subtype();

				if (is_valid_type_promotion(left_type->get_qualities(), right_type->get_qualities())) {
					// pointers are really just integers, so we can
					handle_ss = handle_int_assignment(sym, value, line);
				}
				else {
					// if the pointer subtypes weren't a valid match, throw an exception
					throw TypeDemotionException(line);
				}
				
				break;
			}
            case STRING:
                break;
            case ARRAY:
                break;
            case STRUCT:
                break;
            default:
                throw TypeException(line);
        };

        // mark the symbol as initialized
        sym.set_initialized();
    } else {
        throw TypeException(line);
    }

	return handle_ss;
}

std::stringstream compiler::handle_int_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line) {
    /*

    handle_int_assignment
    Makes an assignment of the value given to the symbol

    @param  symbol  The symbol containing the lvalue
    @param  value   The rvalue
    @return A stringstream containing the generated code

    */

    std::stringstream assign_ss;

    // Generate the code to evaluate the expression; it should go into the a register (rax, eax, ax, or al depending on the data width)
    assign_ss << this->evaluate_expression(value, line).str();
    std::string src = (sym.get_data_type().get_width() == sin_widths::PTR_WIDTH ? "rax" : (sym.get_data_type().get_width() == sin_widths::SHORT_WIDTH ? "ax" : "eax")); // get our source register based on the symbol's width

    // how the variable is allocated will determine how we make the assignment
    if (sym.get_data_type().get_qualities().is_static()) {
        /*
        static variables can be referenced by name
        assignment should look like:
            mov myVar, eax
        */
        assign_ss << "\t" << "mov " << sym.get_name() << ", " << src << std::endl;
    } else if (sym.get_data_type().get_qualities().is_dynamic()) {
        // get pointer for dynamic variable and assign to it
        // the pointer should go in RSI; if it's in use, push it then restore
        bool preserve_rsi = this->reg_stack.peek().is_in_use(RSI);
        if (preserve_rsi) {
            assign_ss << "\t" << "pushq rsi" << std::endl;
        } else {
            // mark the register as in use
            this->reg_stack.peek().set(RSI);
        }

        // make the assignment
        // first, move the pointer to dynamic memory into rsi
        assign_ss << "\t" << "mov rsi, [rbp - " << std::dec << sym.get_stack_offset() << "]" << std::endl;

        // then, assign to the address pointed to by rsi
        assign_ss << "\t" << "mov [rsi], " << src << std::endl;

        // if we had to preserve RSI, restore it
        if (preserve_rsi) {
            assign_ss << "\t" << "popq rsi" << std::endl;
        } else {
            this->reg_stack.peek().clear(RSI);
        }
    } else {
        /*
        automatic memory will write to the appropriate address on the stack for integral types
        simply use the stack offset of the symbol in the assignment, subtracting from rbp
        */
        assign_ss << "\t" << "mov [rbp - " << std::dec << sym.get_stack_offset() << "], " << src << std::endl;
    }

    // return our generated code
    return assign_ss;
}

std::stringstream compiler::handle_bool_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line) {
    /*

    handle_bool_assignment
    Handles an assignment to a boolean variable

    @param  sym The variable's symbol
    @param  value   The expression containing the rvalue
    @param  line    The line number of the expression
    @return A stringstream containing the generated code

    */

    std::stringstream assign_ss;

    // todo: should the language allow implicit conversion between integers and booleans? or not allow any implicit conversions?

    // ensure the types are compatible
    if (sym.get_data_type().is_compatible(get_expression_data_type(value, this->symbol_table, line))) {
        // evaluate the boolean expression -- the result will be in al
        assign_ss << this->evaluate_expression(value, line).str();

        // now, we have to assign based on how the boolean was allocated
        if (sym.get_data_type().get_qualities().is_static()) {
            // assign to named variable
            assign_ss << "\t" << "mov " << sym.get_name() << ", al" << std::endl;
        } else if (sym.get_data_type().get_qualities().is_dynamic()) {
            // get pointer to bool and assign; pointer should go in rsi
            // if RSI is in use, push and restore; else, set to "in use"
            bool restore_rsi = this->reg_stack.peek().is_in_use(RSI);
            if (restore_rsi) {
                assign_ss << "\t" << "pushq rsi" << std::endl;
            } else {
                this->reg_stack.peek().set(RSI);
            }

            // the pointer to dynamic memory is located on the stack
            assign_ss << "\t" << "mov rsi, [rbp - " << std::dec << sym.get_stack_offset() << "]" << std::endl;

            // move the contents of al into the location pointed to by RSI
            assign_ss << "\t" << "mov [rsi], al" << std::endl;

            // restore RSI, if needed
            if (restore_rsi) {
                assign_ss << "\t" << "popq rsi" << std::endl;
            } else {
                // clear the 'in use' status of RSI
                this->reg_stack.peek().clear(RSI);
            }
        } else {
            // automatic memory -- move the contents of al to [rbp - offset]
            assign_ss << "\t" << "mov [rbp - " << sym.get_stack_offset() << "], al" << std::endl;
        }
    } else {
        throw TypeException(line);
    }

    return assign_ss;
}

std::stringstream compiler::handle_string_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line) {
    /*

    handle_string_assignment
    Makes an assignment from one string value to another

    Copy memory from one dynamic location to another. The string will be resized if necessary. 
    Since strings are _not_ references (as they are in Java, for example), a string assignment will _always_ copy the string contents between locations and never re-assign the pointer to the rvalue. If that behavior is desired, use pointers.

    @param  sym The symbol of the lvalue string
    @param  value   The string value to copy
    @param  line    The line number of the assignment
    @return A stringstream containing the generated code

    */

    std::stringstream assign_ss;

    // todo: string assignment

    return assign_ss;
}
