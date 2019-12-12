/*

SIN Toolchain (x86 target)
compile_util/utilities.cpp

Some utility functions for the compiler

*/

#include "utilities.h"

DataType get_expression_data_type(std::shared_ptr<Expression> to_eval, std::unordered_map<std::string, std::shared_ptr<symbol>> &symbol_table, unsigned int line) {
    /*

    get_expression_data_type
    Evaluates the data type of an expression

    @param  to_eval The expression we want to evaluate
    @return A DataType object containing the type information

    */
   
    DataType type_information;

    // we will fetch the data type for the expression based on the expression type
    exp_type expression_type = to_eval->get_expression_type();

    switch (expression_type) {
        case LITERAL:
        {
            // set base type data
            Literal *literal = dynamic_cast<Literal*>(to_eval.get());
            type_information = literal->get_data_type();
            break;
        }
        // since indexed inherits from lvalue, and we are just getting types, we can put them in the same case
        case LVALUE:
        case INDEXED:
        {
            // look into the symbol table for an LValue
            LValue *lvalue = dynamic_cast<LValue*>(to_eval.get());
            std::unordered_map<std::string, std::shared_ptr<symbol>>::iterator it = symbol_table.find(lvalue->getValue());

            // if the symbol isn't in the table, throw an exception; else, continue
            if (it == symbol_table.end()) {
                throw SymbolNotFoundException(line);
            } else {
                // get the symbol and return its type data
                std::shared_ptr<symbol> sym = it->second;
                type_information = sym->get_data_type();
            }
            break;
        }
        case LIST:
        {
            // get list type
            ListExpression *init_list = dynamic_cast<ListExpression*>(to_eval.get());
            
            // A list expression is a vector of other expressions, get the first item and pass it into this function recursively
            DataType sub_data_type = get_expression_data_type(init_list->get_list()[0], symbol_table, line);

            // the subtype will be the current primary type, and the primary type will be array
            sub_data_type.set_subtype(sub_data_type.get_primary());
            sub_data_type.set_primary(ARRAY);

            // copy it into type_information
            type_information = sub_data_type;
            break;
        }
        case ADDRESS_OF:
            // get the pointer
            // pointers are always unsigned long ints
            type_information.set_primary(INT);
            type_information.add_qualities(std::vector<SymbolQuality> { UNSIGNED, LONG });
            break;
        case DEREFERENCED:
        {
            // get the type of the dereferenced pointer
            Dereferenced *deref = dynamic_cast<Dereferenced*>(to_eval.get());
            
            // Dereferenced expressions contain a pointer to another expression; get its type
            type_information = get_expression_data_type(deref->get_ptr_shared(), symbol_table, line);
            break;
        }
        case BINARY:
        {
            // get the type of a binary expression
            break;
        }
        case UNARY:
        {
            // get the type of a unary expression
            break;
        }
        case VALUE_RETURNING_CALL:
        {
            // look into the symbol table to get the return type of the function
            ValueReturningFunctionCall *call_exp = dynamic_cast<ValueReturningFunctionCall*>(to_eval.get());

            break;
        }
        case SIZE_OF:
            // size_of always returns a const unsigned integer type
            type_information.set_primary(INT);
            type_information.add_qualities(std::vector<SymbolQuality>{CONSTANT, UNSIGNED});
            break;
        default:
            // todo: throw an exception if the type is invalid
            break;
    };

    return type_information;
}
