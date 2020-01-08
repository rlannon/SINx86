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
            Binary *binary = dynamic_cast<Binary*>(to_eval.get());

            /*

            Binary expressions are a little more tricky because they can involve multiple operands of different types

            We must get the types of the left and right operands and compare them. The qualifiers (including sizes) might change:
                - If one operand is signed, and the other is unsigned, the result may or may not be signed; it will generate a 'signed/unsigned mismatch' warning
                - The width will change to match the widest operand
            
            In order to determine these operand types, this function is called recursively

            */

            DataType left = get_expression_data_type(binary->get_left(), symbol_table, line);
            DataType right = get_expression_data_type(binary->get_right(), symbol_table, line);

            // ensure the types are compatible
            if (left.is_compatible(right)) {
                if (left.get_width() >= right.get_width()) {
                    type_information = left;
                } else {
                    type_information = right;
                }
            } else {
                throw TypeException(line);  // throw an exception if the types are not compatible with one another
            }

            break;
        }
        case UNARY:
        {
            // get the type of a unary expression
            Unary *u = dynamic_cast<Unary*>(to_eval.get());

            // Unary expressions contain an expression inside of them; call this function recursively using said expression as a parameter
            type_information = get_expression_data_type(u->get_operand(), symbol_table, line);
            break;
        }
        case VALUE_RETURNING_CALL:
        {
            // look into the symbol table to get the return type of the function
            ValueReturningFunctionCall *call_exp = dynamic_cast<ValueReturningFunctionCall*>(to_eval.get());
            std::unordered_map<std::string, std::shared_ptr<symbol>>::iterator it = symbol_table.find(call_exp->get_func_name());

            // make sure it's in the table
            if (it == symbol_table.end()) {
                throw SymbolNotFoundException(line);
            } else {
                // ensure the symbol is a function symbol
                if (it->second->get_symbol_type() == FUNCTION_SYMBOL) {
                    // get the function symbol
                    function_symbol *func_sym = dynamic_cast<function_symbol*>(it->second.get());

                    // get the return type data
                    type_information = func_sym->get_data_type();
                } else {
                    throw InvalidSymbolException(line);
                }
            }
            break;
        }
        case SIZE_OF:
            // size_of always returns a const unsigned integer type
            type_information.set_primary(INT);
            type_information.add_qualities(std::vector<SymbolQuality>{CONSTANT, UNSIGNED});
            break;
        default:
            throw CompilerException("Invalid expression type", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
            break;
    };

    return type_information;
}

bool can_pass_in_register(DataType to_check) {
    // Checks whether the given DataType can be passed in a register or if it must be passed on the stack

    Type primary = to_check.get_primary();
    return (
        primary != ARRAY &&
        primary != STRUCT &&
        primary != STRING
    );
}

struct_info define_struct(StructDefinition definition) {
    /*
    
    define_struct
    Creates a struct_info object based on a syntax tree for a struct definition

    Since this doens't actually affect our symbol table, we don't need compiler members
    Note that the caller should ensure that the definition statement occurs within the globa scope -- it is not the responsibility of this function

    @param  definition  The definition statement for the struct
    @return A 'struct_info' object which may be added to the compiler's struct table
    @throws Throws a StructDefinitionException if there are statements other than

    */

    // get the struct's name
    std::string struct_name;
    LValue *name_lval = dynamic_cast<LValue*>(definition.get_name().get());
    struct_name = name_lval->getValue();    // todo: refactor name to be a string in 'Definition'

    // iterate through our definition statements and create symbols for all struct members
    std::vector<symbol> members;
    size_t current_offset = 0;
    for (std::shared_ptr<Statement> s: definition.get_procedure()->statements_list) {
        // Only allocations are allowed within a struct body
        if (s->get_statement_type() == ALLOCATION) {
            // cast to Allocation and create a symbol
            Allocation *alloc = dynamic_cast<Allocation*>(s.get());
            symbol sym(alloc->get_var_name(), struct_name, 1, alloc->get_type_information(), current_offset);
            
            // todo: allow default values (alloc-init syntax) in structs?

            // add that symbol to our vector
            members.push_back(sym);

            // update the data offset
            // todo: how to handle struct or array members? allocate space for a pointer?
            current_offset += alloc->get_type_information().get_width();
        } else {
            throw StructDefinitionException(definition.get_line_number());
        }
    }

    // construct and return a struct_info object
    return struct_info(struct_name, members, definition.get_line_number());
}
