/*

SIN Toolchain (x86 target)
assign.cpp

Implementation of the assignment functions for the compiler

*/

#include "compile.h"

std::stringstream compiler::assign(Assignment assign_stmt) {
    /*

    assign
    Generates code for an assignment statement

    This function dispatches the work for making assignments to the appropriate functions

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
    } else if (sym.get_symbol_type() == FUNCTION_DEFINITION) {
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

    @param  sym The symbol to which we are assigning data
    @param  value   A shared pointer to the expression for the assignment
    @return A stringstream containing the generated code

    */

    // First, we need to determine some information about the symbol
    // Look at its type and dispatch accordingly
    DataType symbol_type = sym.get_data_type();
    DataType expression_type = get_expression_data_type(value, this->symbol_table, line);

    // ensure our expression's data type is compatible with our variable's data type
    if (symbol_type.is_compatible(expression_type)) {
        // dispatch appropriately based on the data type
        if (symbol_type.get_primary() == INT) {
            return handle_int_assignment(sym, value, line);
        } else if (symbol_type.get_primary() == CHAR) {

        } else if (symbol_type.get_primary() == FLOAT) {

        } else if (symbol_type.get_primary() == BOOL) {

        } else if (symbol_type.get_primary() == PTR) {

        } else if (symbol_type.get_primary() == STRING) {

        } else if (symbol_type.get_primary() == ARRAY) {

        } else if (symbol_type.get_primary() == STRUCT) {

        }
    } else {
        throw TypeException(line);
    }
}

std::stringstream compiler::handle_int_assignment(symbol &symbol, std::shared_ptr<Expression> value, unsigned int line) {
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

    // todo: make assignment

    // todo: how the variable is allocated will determine how we make the assignment

    // return our generated code
    return assign_ss;
}
