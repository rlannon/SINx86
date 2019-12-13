/*

SIN Toolchain (x86 target)
evaluate_expression.cpp

Generates code for evaluating an expression. This data will be loaded into registers as specified by "Doc/Registers"

*/

#include "utilities.h"

std::stringstream evaluate_expression(std::shared_ptr<Expression> value, std::unordered_map<std::string, symbol> &symbol_table, unsigned int line) {
    /*

    evaluate_expression
    Generates code to evaluate a given expression

    Generates code to evaluate an expression, keeping the result in the A register (could be AL, AX, EAX, or RAX) if possible

    @param  value   The expression to be evaluated
    @return A stringstream containing the generated code

    */

    // todo: track which registers are in use?

    std::stringstream evaluation_ss;

    // The expression evaluation depends on the expression's type
    switch (value->get_expression_type()) {
        case LITERAL:
        {
            break;
        }
        case LVALUE:
        {
            break;
        }
        case INDEXED:
        {
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
            break;
        }
        case UNARY:
        {
            break;
        }
        case VALUE_RETURNING_CALL:
        {
            break;
        }
        case SIZE_OF:
        {
            break;
        }
        default:
            throw CompilerException("Invalid expression type", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
    }

    return evaluation_ss;
}
