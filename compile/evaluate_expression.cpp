/*

SIN Toolchain (x86 target)
evaluate_expression.cpp

Generates code for evaluating an expression. This data will be loaded into registers as specified by "Doc/Registers"

*/

#include "compile.h"

std::stringstream compiler::evaluate_expression(std::shared_ptr<Expression> to_evaluate, unsigned int line) {
    /*

    evaluate_expression
    Generates code to evaluate a given expression

    Generates code to evaluate an expression, keeping the result in the A register (could be AL, AX, EAX, or RAX) if possible.
    If the size of the object does not permit it to passed in a register, then it will be returned on the stack.

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
            Literal &literal_exp = *dynamic_cast<Literal*>(to_evaluate.get());

            // dispatch to our evaluation function
            evaluation_ss = this->evaluate_literal(literal_exp, line);
            break;
        }
        case LVALUE:
        {
            // get the lvalue
            LValue &lvalue_exp = *dynamic_cast<LValue*>(to_evaluate.get());

            // dispatch to our evaluation function
            evaluation_ss << this->evaluate_lvalue(lvalue_exp, line);
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

std::stringstream compiler::evaluate_literal(Literal &to_evaluate, unsigned int line) {
    /*

    Evaluates a literal expression

    */

    std::stringstream eval_ss;

    // act based on data type and width
    DataType type = to_evaluate.get_data_type();
    if (type.get_primary() == INT) {
        /*

        short ints are 16 bits wide and get loaded into ax
        normal-width ints are 32 bits wide and get loaded into eax
        long ints are 64 bits wide and get loaded into rax

        */
        
        if (type.get_width() == 16) {
            eval_ss << "\t" << "mov ax, " << to_evaluate.get_value() << std::endl;
        } else if (type.get_width() == 32) {
            eval_ss << "\t" << "mov eax, " << to_evaluate.get_value() << std::endl;
        } else if (type.get_width() == 64) {
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
        data << name << " .dd " << to_evaluate.get_value().length() << "`" << to_evaluate.get_value() << "`" << std::endl;
        this->strc_num += 1;

        // now, make sure our strings goes into the DATA section
        this->data_segment << data.str();

        // now, load the a register with the address of the string
        eval_ss << "mov rax, " << name << std::endl;
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
    Result will be returned on A, as usual

    */

    // todo: evaluate lvalues
}
