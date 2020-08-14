/*

SIN toolchain (x86 target)
expression_util.cpp
Copyright 2020 Riley Lannon

Implements the expression utility functions given in the associated header

*/

#include "expression_util.h"

std::stringstream expression_util::get_exp_address(
    std::shared_ptr<Expression> exp,
    symbol_table &symbols,
    struct_table &structs,
    reg r,
    unsigned int line
) {
    /*

    Gets the address of an expression

    */

    std::stringstream addr_ss;

    std::string r_name = register_usage::get_register_name(r);

    if (exp->get_expression_type() == IDENTIFIER) {
        // we have a utility for these already
        auto l = dynamic_cast<Identifier*>(exp.get());
        auto sym = symbols.find(l->getValue());
        addr_ss << get_address(*sym, r);
    }
    else if (exp->get_expression_type() == UNARY) {
        // use this function recursively to get the address of the operand
        auto u = dynamic_cast<Unary*>(exp.get());
        addr_ss << get_exp_address(u->get_operand(), symbols, structs, r, line).str();
        
        if (u->get_operator() == DEREFERENCE) {
            // dereference the pointer we fetched
            addr_ss << "\t" << "mov " << r_name << ", [" << r_name << "]" << std::endl;
        }
    }
    else if (exp->get_expression_type() == INDEXED) {
        // use recursion
        auto i = dynamic_cast<Indexed*>(exp.get());
        addr_ss << get_exp_address(i->get_to_index(), symbols, structs, r, line).str();
        // note that this can't evaluate the index; that's up to the caller
    }
    else if (exp->get_expression_type() == BINARY) {
        // create and evaluate a member_selection object
        auto b = dynamic_cast<Binary*>(exp.get());
        member_selection m = member_selection::create_member_selection(*b, structs, symbols, line);
        addr_ss << m.evaluate(symbols, structs, line).str();

        // make sure the address goes into the register we selected (the 'evaluate' function returns the address of the member RBX)
        if (r != RBX) {
            addr_ss << "\t" << "mov " << r_name << ", rbx" << std::endl;
        }
    }

    return addr_ss;
}

DataType expression_util::get_expression_data_type(
    std::shared_ptr<Expression> to_eval,
    symbol_table& symbols,
    struct_table& structs,
    unsigned int line
) {
    /*

    get_expression_data_type
    Evaluates the data type of an expression

    @param  to_eval The expression we want to evaluate
    @return A DataType object containing the type information

    */

	// todo: ensure that with floating-point expressions, if a double-precision float is used, the result has a width of DOUBLE_WIDTH
    // todo: write function to handle evaluation of dot operator expressions
   
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
        case IDENTIFIER:
		{
            // look into the symbol table for an LValue
            Identifier *ident = dynamic_cast<Identifier*>(to_eval.get());
			std::shared_ptr<symbol> sym;

			try {
				// get the symbol and return its type data
				sym = symbols.find(ident->getValue());
			}
			catch (std::exception& e) {
				throw SymbolNotFoundException(line);
			}

            // the expression type of a reference should be treated as its subtype
            if (sym->get_data_type().get_primary() == REFERENCE) {
                type_information = sym->get_data_type().get_subtype();
            }
            else {
                type_information = sym->get_data_type();
            }

            break;
        }
        case INDEXED:
        {
            Indexed *idx = dynamic_cast<Indexed*>(to_eval.get());
            DataType t = expression_util::get_expression_data_type(idx->get_to_index(), symbols, structs, line);
            // we can index strings or arrays; if we index an array, we get the subtype, and if we index a string, we get a char
            if (t.get_primary() == ARRAY) {
                type_information = t.get_subtype();
            }
            else if (t.get_primary() == STRING) {
                type_information = DataType(
                    Type::CHAR
                );
            }
            break;
        }
        case LIST:
        {
            // get list type
            ListExpression *init_list = dynamic_cast<ListExpression*>(to_eval.get());
            
            // iterate over the elements and get the data types of each
            bool homogeneous = true;    // if homogeneous, we will set the type as array; else, tuple (tuple assignment checks each type against every other)
            std::vector<DataType> contained_types;
            for (auto item: init_list->get_list()) {
                DataType dt = expression_util::get_expression_data_type(
                    item,
                    symbols,
                    structs,
                    line
                );
                contained_types.push_back(dt);
                
                // check the homogeneity of the list
                if (homogeneous) {
                    if (contained_types.front() != dt)
                        homogeneous = false;
                }
            }

            // the subtype will be the current primary type, and the primary type will be array
            type_information.set_contained_types(contained_types);
            type_information.set_primary(homogeneous ? ARRAY : TUPLE);

            if (!homogeneous && init_list->get_list_type() == ARRAY) {
                throw CompilerException(
                    "Array list expressions must be homogeneous",
                    compiler_errors::LIST_TYPE_MISMATCH,
                    line
                );
            }
            
            break;
        }
        case BINARY:
        {
            // get the type of a binary expression
            Binary *binary = dynamic_cast<Binary*>(to_eval.get());

            /*

            Binary expressions are a little more tricky because they can involve multiple operands of different types
			Further, some operators (the (in)equality operators) return different types than their operands
            Note, however, that dot operator expressions must be handled differently (via member_selection) than others because they require a bit more involvement.

            We must get the types of the left and right operands and compare them. The qualifiers (including sizes) might change:
                - If one operand is signed, and the other is unsigned, the result may or may not be signed; it will generate a 'signed/unsigned mismatch' warning
                - The width will change to match the widest operand
            
            In order to determine these operand types, this function is called recursively

            */

            if (binary->get_operator() == exp_operator::DOT) {
                // create a member_selection object for the expression
                member_selection m(*binary, structs, symbols, line);

                // now, just look at the data type of the last node
                type_information = m.last().get_data_type();
            } else {
                // get both expression types
                DataType left = expression_util::get_expression_data_type(binary->get_left(), symbols, structs, line);
                DataType right = expression_util::get_expression_data_type(binary->get_right(), symbols, structs, line);

                // ensure the types are compatible
                if (left.is_compatible(right)) {
                    // check for in/equality operators -- these will return booleans instead of the original type!
                    exp_operator op = binary->get_operator();
                    if (op == EQUAL || op == NOT_EQUAL || op == GREATER || op == GREATER_OR_EQUAL || op == LESS || op == LESS_OR_EQUAL) {
                        type_information = DataType(BOOL, DataType(), symbol_qualities());
                    }
                    else {
                        if (left.get_width() >= right.get_width()) {
                            type_information = left;
                        }
                        else {
                            type_information = right;
                        }
                    }
                } else {
                    throw TypeException(line);  // throw an exception if the types are not compatible with one another
                }
            }

            break;
        }
        case UNARY:
        {
            // get the type of a unary expression
            Unary *u = dynamic_cast<Unary*>(to_eval.get());

            // Unary expressions contain an expression inside of them; call this function recursively using said expression as a parameter
            type_information = expression_util::get_expression_data_type(u->get_operand(), symbols, structs, line);

            // if the operator is ADDRESS, we need to wrap the type information in a pointer
            if (u->get_operator() == ADDRESS) {
                auto full_subtype = DataType(type_information);
                type_information = DataType(PTR);
                type_information.set_subtype(full_subtype);
            }
            // if the operator is DEREFERENCE, we need to *remove* the pointer type
            else if (u->get_operator() == DEREFERENCE) {
                type_information = type_information.get_subtype();
            }

            break;
        }
        case VALUE_RETURNING_CALL:
        {
            // look into the symbol table to get the return type of the function
            ValueReturningFunctionCall *call_exp = dynamic_cast<ValueReturningFunctionCall*>(to_eval.get());
			std::shared_ptr<symbol> sym = symbols.find(call_exp->get_func_name());

            // ensure the symbol is a function symbol
            if (sym->get_symbol_type() == FUNCTION_SYMBOL) {
                // get the function symbol
                function_symbol *func_sym = dynamic_cast<function_symbol*>(sym.get());

                // get the return type data
                type_information = func_sym->get_data_type();
            } else {
                throw InvalidSymbolException(line);
            }
            break;
        }
        case CAST:
        {
            Cast *c = dynamic_cast<Cast*>(to_eval.get());
            if (DataType::is_valid_type(c->get_new_type())) {
                type_information = c->get_new_type();
            }
            else {
                throw CompilerException("Attempt to cast to invalid type", compiler_errors::INVALID_CAST_ERROR, line);
            }
            break;
        }
        case ATTRIBUTE:
        {
            auto attr = dynamic_cast<AttributeSelection*>(to_eval.get());
            // todo: should attributes always return integers?
            type_information.set_primary(INT);
            type_information.add_qualities(
                std::vector<SymbolQuality>{
                    CONSTANT,
                    UNSIGNED
                }
            );
            break;
        }
        default:
            throw CompilerException("Invalid expression type", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
            break;
    };

    return type_information;
}

