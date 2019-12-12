/*

SIN Toolchain (x86 target)
assign.cpp

Implementation of the assignment functions for the compiler

*/

#include "compile.h"
#include "../util/CompilerErrorCodes.h"

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
    std::unordered_map<std::string, std::shared_ptr<symbol>>::iterator it = this->symbol_table.find(lvalue->getValue());

    // if we could find the symbol, perform the assignment; else, throw an exception
    if (it != this->symbol_table.end()) {
        // ensure we aren't assigning to a const-qualified variable
        if (it->second->get_data_type().get_qualities().is_const()) {
            throw ConstAssignmentException(assign_stmt.get_line_number());
        } else {
            // dispatch to the assignment handler
            return this->handle_assignment(*(it->second.get()), assign_stmt.get_rvalue(), assign_stmt.get_line_number());
        }
    } else {
        throw SymbolNotFoundException(assign_stmt.get_line_number());
    }
}

std::stringstream compiler::handle_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line) {
    /*

    handle_assignment
    Generates code to assign 'value' to 'sym'

    @param  sym The symbol to which we are assigning data
    @param  value   A shared pointer to the expression for the assignment
    @return A stringstream containing the generated code

    */

    // First, we need to determine some information about the symbol
    // Look at its type and dispatch accordingly
    DataType symbol_type = sym.get_data_type();
    DataType expression_type = get_expression_data_type(value, this->symbol_table, line);

    if (symbol_type.get_primary() == INT) {

    } else if (symbol_type.get_primary() == FLOAT) {

    } else if (symbol_type.get_primary() == BOOL) {

    } else if (symbol_type.get_primary() == PTR) {

    } else if (symbol_type.get_primary() == STRING) {

    } else if (symbol_type.get_primary() == ARRAY) {

    } else if (symbol_type.get_primary() == STRUCT) {

    }
}
