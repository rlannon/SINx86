/*

SIN toolchain (x86 target)
expression_util.cpp
Copyright 2020 Riley Lannon

Implements the expression utility functions given in the associated header

*/

#include "expression_util.h"

std::stringstream expression_util::get_exp_address(
    const Expression &exp,
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

    if (exp.get_expression_type() == IDENTIFIER) {
        // we have a utility for these already
        auto &l = static_cast<const Identifier&>(exp);
        auto &sym = symbols.find(l.getValue());
        addr_ss << get_address(sym, r);
    }
    else if (exp.get_expression_type() == UNARY) {
        // use this function recursively to get the address of the operand
        auto &u = static_cast<const Unary&>(exp);
        addr_ss << get_exp_address(u.get_operand(), symbols, structs, r, line).str();
        
        if (u.get_operator() == DEREFERENCE) {
            // dereference the pointer we fetched
            addr_ss << "\t" << "mov " << r_name << ", [" << r_name << "]" << std::endl;
        }
    }
    else if (exp.get_expression_type() == INDEXED) {
        // use recursion
        auto &i = static_cast<const Indexed&>(exp);
        addr_ss << get_exp_address(i.get_to_index(), symbols, structs, r, line).str();
        // note that this can't evaluate the index; that's up to the caller
    }
    else if (exp.get_expression_type() == BINARY) {
        // create and evaluate a member_selection object
        auto &b = static_cast<const Binary&>(exp);
        addr_ss << expression_util::evaluate_member_selection(b, symbols, structs, r, line, false).str();
    }

    return addr_ss;
}

std::stringstream expression_util::evaluate_member_selection(
    const Binary &to_evaluate,
    symbol_table &symbols,
    struct_table &structs,
    reg r,
    unsigned int line,
    bool dereference
) {
    /*

    evaluate_member_selection
    Evaluates a dot operator expression (a simple binary)

    */

    std::stringstream eval_ss;

    auto reg_name = register_usage::get_register_name(r);
    DataType result_type;

    // evaluate the lhs -- get the address of the lhs in RBX
    auto lhs_type = expression_util::get_expression_data_type(to_evaluate.get_left(), symbols, structs, line);
    eval_ss << expression_util::get_exp_address(to_evaluate.get_left(), symbols, structs, r, line).str();

    // evaluate the rhs -- get the offset from the lhs type
    if (lhs_type.get_primary() == STRUCT) {
        auto lhs_struct = structs.find(lhs_type.get_struct_name(), line);
        size_t member_offset = 0;

        // structs must be accessed with an identifier -- other expression types are syntactically invalid
        if (to_evaluate.get_right().get_expression_type() == IDENTIFIER) {
            auto &id = static_cast<const Identifier&>(to_evaluate.get_right());
            auto member = lhs_struct.get_member(id.getValue());
            member_offset = member->get_offset();

            if (member_offset > 0) {
                eval_ss << "\t" << "add " << reg_name << ", " << member_offset << std::endl;
            }
            result_type = member->get_data_type();

        }
        else if (to_evaluate.get_right().get_expression_type() == CALL_EXP) {
            auto &method = static_cast<const CallExpression&>(to_evaluate.get_right());

            // todo: method calls
        }
        else {
            throw CompilerException(
                "Struct members must be accessed with an identifier",
                compiler_errors::STRUCT_MEMBER_SELECTION_ERROR,
                line
            );
        }
    }
    else if (lhs_type.get_primary() == TUPLE) {
        // get the offset for our index
        size_t member_offset = 0;
        if (to_evaluate.get_right().get_expression_type() == LITERAL) {
            auto &lit = static_cast<const Literal&>(to_evaluate.get_right());
            if (lit.get_data_type().get_primary() != INT) {
                throw CompilerException(
                    "Expected integer literal",
                    compiler_errors::TUPLE_MEMBER_SELECTION_ERROR,
                    line
                );
            }

            unsigned long member_number = std::stoul(lit.get_value());
            if (member_number < lhs_type.get_contained_types().size()) {
                for (size_t i = 0; i < member_number; i++) {
                    member_offset += lhs_type.get_contained_types().at(i).get_width();
                }
                result_type = lhs_type.get_contained_types().at(member_number);
            }
            else {
                throw CompilerException(
                    "Member out of bounds",
                    compiler_errors::OUT_OF_BOUNDS,
                    line
                );
            }
        }
        else {
            throw CompilerException(
                "Tuple members must be accessed with an integer literal",
                compiler_errors::TUPLE_MEMBER_SELECTION_ERROR,
                line
            );
        }

        if (member_offset > 0) {
            eval_ss << "\t" << "add " << reg_name << ", " << member_offset << std::endl;
        }
    }
    else {
        throw CompilerException(
            "Expected left-hand expression of tuple or struct type",
            compiler_errors::STRUCT_TYPE_EXPECTED_ERROR,
            line
        );
    }

    // pass the value in a register if we can (and want to)
    if (dereference && can_pass_in_register(result_type)) {
        eval_ss << "\t" << "mov " << register_usage::get_register_name(r, result_type) << ", [" << reg_name << "]" << std::endl;
    }

    return eval_ss;
}

DataType expression_util::get_expression_data_type(
    const Expression &to_eval,
    symbol_table& symbols,
    struct_table& structs,
    unsigned int line,
    const DataType *type_hint
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
    exp_type expression_type = to_eval.get_expression_type();

    switch (expression_type) {
        case LITERAL:
        {
            // set base type data
            auto &literal = static_cast<const Literal&>(to_eval);
            type_information = literal.get_data_type();

            // update qualities based on type hint, if applicable
            if (type_hint) {
                if (type_information.get_primary() == type_hint->get_primary()) {
                    type_information = *type_hint;
                }
            }

            break;
        }
        case IDENTIFIER:
		{
            // look into the symbol table for an LValue
            auto &ident = static_cast<const Identifier&>(to_eval);
			symbol *sym = nullptr;

			try {
				// get the symbol and return its type data
				sym = &symbols.find(ident.getValue());
			}
			catch (SymbolNotFoundException& e) {
				e.set_line(line);
                throw e;
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
            auto &idx = static_cast<const Indexed&>(to_eval);
            DataType t = expression_util::get_expression_data_type(idx.get_to_index(), symbols, structs, line);
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
            auto &init_list = static_cast<const ListExpression&>(to_eval);
            
            // iterate over the elements and get the data types of each
            bool homogeneous = true;    // if homogeneous, we will set the type as array; else, tuple (tuple assignment checks each type against every other)
            std::vector<DataType> contained_types;
            for (auto item: init_list.get_list()) {
                DataType dt = expression_util::get_expression_data_type(
                    *item,
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

            if (!homogeneous && init_list.get_list_type() == ARRAY) {
                throw CompilerException(
                    "Array list expressions must be homogeneous",
                    compiler_errors::LIST_TYPE_MISMATCH,
                    line
                );
            }
            else if (init_list.get_list_type() == ARRAY) {
                size_t new_length = contained_types.size();
                new_length *= contained_types.at(0).get_width();
                new_length += sin_widths::INT_WIDTH;
                
                type_information.set_array_length(new_length);
            }

            // the subtype will be the current primary type, and the primary type will be array
            type_information.set_contained_types(contained_types);
            type_information.set_primary(init_list.get_list_type());
            
            break;
        }
        case BINARY:
        {
            // get the type of a binary expression
            auto &binary = static_cast<const Binary&>(to_eval);

            /*

            Binary expressions are a little more tricky because they can involve multiple operands of different types
			Further, some operators (the (in)equality operators) return different types than their operands
            Note, however, that dot operator expressions must be handled differently (via member_selection) than others because they require a bit more involvement.

            We must get the types of the left and right operands and compare them. The qualifiers (including sizes) might change:
                - If one operand is signed, and the other is unsigned, the result may or may not be signed; it will generate a 'signed/unsigned mismatch' warning
                - The width will change to match the widest operand
            
            In order to determine these operand types, this function is called recursively

            */

            if (binary.get_operator() == exp_operator::DOT) {
                // get the data type of the left side to get the struct name or tuple type information
                auto lhs_type = expression_util::get_expression_data_type(binary.get_left(), symbols, structs, line);
                if (lhs_type.get_primary() == STRUCT) {
                    auto &lhs_struct = structs.find(lhs_type.get_struct_name(), line);
                    if (binary.get_right().get_expression_type() == IDENTIFIER) {
                        auto &r = static_cast<const Identifier&>(binary.get_right());
                        type_information = lhs_struct.get_member(r.getValue())->get_data_type();
                    }
                    // todo: exception
                }
                else if (lhs_type.get_primary() == TUPLE) {
                    if (binary.get_right().get_expression_type() == LITERAL) {
                        auto &r = static_cast<const Literal&>(binary.get_right());
                        if (r.get_data_type().get_primary() == INT) {
                            unsigned long index_value = std::stoul(r.get_value());
                            if (index_value < lhs_type.get_contained_types().size()) {
                                type_information = lhs_type.get_contained_types().at(index_value);
                            }
                            else {
                                throw CompilerException("Tuple member selection out of bounds", compiler_errors::OUT_OF_BOUNDS, line);
                            }
                        }
                        else {
                            throw TypeException(line);
                        }
                    }
                    // todo: exception
                }
                else {
                    throw IllegalMemberSelectionType(line);
                }
            } else {
                // get both expression types
                DataType left = expression_util::get_expression_data_type(binary.get_left(), symbols, structs, line);
                DataType right = expression_util::get_expression_data_type(binary.get_right(), symbols, structs, line);

                // ensure the types are compatible
                if (left.is_compatible(right)) {
                    // check for in/equality operators -- these will return booleans instead of the original type!
                    exp_operator op = binary.get_operator();
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
            auto &u = static_cast<const Unary&>(to_eval);

            // Unary expressions contain an expression inside of them; call this function recursively using said expression as a parameter
            type_information = expression_util::get_expression_data_type(u.get_operand(), symbols, structs, line);

            // if the operator is ADDRESS, we need to wrap the type information in a pointer
            if (u.get_operator() == ADDRESS) {
                auto full_subtype = DataType(type_information);
                type_information = DataType(PTR);
                type_information.set_subtype(full_subtype);
            }
            // if the operator is DEREFERENCE, we need to *remove* the pointer type
            else if (u.get_operator() == DEREFERENCE) {
                type_information = type_information.get_subtype();
            }

            break;
        }
        case CALL_EXP:
        {
            // look into the symbol table to get the return type of the function
            auto &call_exp = static_cast<const CallExpression&>(to_eval);
            symbol &sym = expression_util::get_function_symbol(
                call_exp.get_func_name(),
                structs,
                symbols,
                line
            );

            // ensure the symbol is a function symbol
            if (sym.get_symbol_type() == FUNCTION_SYMBOL) {
                // get the function symbol
                function_symbol &func_sym = static_cast<function_symbol&>(sym);

                // get the return type data
                type_information = func_sym.get_data_type();
            } else {
                throw InvalidSymbolException(line);
            }
            break;
        }
        case CAST:
        {
            auto &c = static_cast<const Cast&>(to_eval);
            if (DataType::is_valid_type(c.get_new_type())) {
                type_information = c.get_new_type();
            }
            else {
                throw CompilerException("Attempt to cast to invalid type", compiler_errors::INVALID_CAST_ERROR, line);
            }
            break;
        }
        case ATTRIBUTE:
        {
            type_information.set_primary(INT);
            type_information.add_qualities(
                std::vector<SymbolQuality>{
                    CONSTANT,
                    UNSIGNED
                }
            );
            break;
        }
        case CONSTRUCTION_EXP:
        {
            /*

            The construction expression may have an explicit type; otherwise, 
            use the datatype we passed in
            
            Regardless, a construction expression will always yield a structure 
            (it cannot be used with primitives)

            */

            type_information.set_primary(STRUCT);
            
            auto &ctor = static_cast<const Construction&>(to_eval);
            if (ctor.has_explicit_type())
            {
                type_information.set_struct_name(ctor.get_explicit_type());
            }
            else if (type_hint)
            {
                type_information = *type_hint;
            }
            else
            {
                throw CompilerException(
                    "Cannot deduce constructed type",
                    compiler_errors::CONSTRUCTION_TYPE_DEDUCTION_FAILURE,
                    line
                );
            }

            break;
        }
        default:
            throw CompilerException("Invalid expression type", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
            break;
    };

    return type_information;
}

size_t expression_util::get_width(
    DataType &alloc_data,
    compile_time_evaluator &evaluator,
    struct_table &structs,
    symbol_table &symbols,
    std::string scope_name,
    unsigned int scope_level,
    unsigned int line
) {
    /*

    get_width
    Gets the width of the given data type

    */

    size_t width = 0;

    if (alloc_data.get_width() != 0) {
        width = alloc_data.get_width();
    }
    else if (alloc_data.get_primary() == STRUCT) {
        struct_info &s = structs.find(alloc_data.get_struct_name(), line);
        width = s.get_width();
    }
    else if (alloc_data.get_primary() == ARRAY) {
        // if we have an array length expression, and it's a constant, evaluate it
		if (alloc_data.get_array_length_expression() != nullptr && alloc_data.get_array_length_expression()->is_const()) {
			if (
				expression_util::get_expression_data_type(
					*alloc_data.get_array_length_expression(),
					symbols,
					structs,
					line
				).get_primary() == INT)
			{
				// pass the expression to our expression evaluator to get the array width
				// todo: compile-time evaluation
				// todo: set alloc_data::array_length
				alloc_data.set_array_length(
                    stoul(
                        evaluator.evaluate_expression(
                            *alloc_data.get_array_length_expression(),
                            scope_name,
                            scope_level,
                            line
                        )
                    )
                );
				width = alloc_data.get_array_length() * alloc_data.get_subtype().get_width() + sin_widths::INT_WIDTH;
			}
			else {
				throw NonConstArrayLengthException(line);
			}
		}
        else if (alloc_data.get_array_length() != 0) {
            width = alloc_data.get_array_length();
        }
		else {
			// if the length is not constant, check to see if we have a dynamic array; if not, then it's not legal
			if (alloc_data.get_qualities().is_dynamic()) {
				alloc_data.set_array_length(0);
				width = sin_widths::PTR_WIDTH;
			}
			else {
				throw NonConstArrayLengthException(line);
			}
		}
    }
    else if (alloc_data.get_primary() == TUPLE) {
        for (auto it = alloc_data.get_contained_types().begin(); it != alloc_data.get_contained_types().end(); it++) {
            if (it->get_width() == 0) {
				width += get_width(*it, evaluator, structs, symbols, scope_name, scope_level, line);
            }
            else {
                width += it->get_width();
            }
        }
    }

    return width;
}

symbol &expression_util::get_function_symbol(
    const Expression &func_name,
    struct_table &structs,
    symbol_table &symbols,
    unsigned int line
) {
    /*

    get_function_symbol
    Gets a reference to the function_symbol referenced by the func_name Expression reference

    */

    // todo: consider how this lookup scheme will work for proc objects

    exp_type name_exp_type = func_name.get_expression_type();
    switch(name_exp_type) {
        case IDENTIFIER:
        {
            auto &id = static_cast<const Identifier&>(func_name);
            try {
                auto &s = symbols.find(id.getValue());
                return s;
            }
            catch (SymbolNotFoundException &e) {
                e.set_line(line);
                throw e;
            }
        }
        case BINARY:
        {
            symbol *s = nullptr;
            auto &bin = static_cast<const Binary&>(func_name);
            if (bin.get_operator() == DOT) {
                auto &lhs_struct = expression_util::get_struct_type(bin.get_left(), structs, symbols, line);
                try {
                    auto &id = static_cast<const Identifier&>(bin.get_right());
                    s = lhs_struct.get_member(id.getValue());
                }
                catch (std::bad_cast &e) {
                    throw CompilerException(
                        "Expected valid struct member name",
                        compiler_errors::INVALID_EXPRESSION_TYPE_ERROR,
                        line
                    );
                }
                catch (SymbolNotFoundException &e) {
                    e.set_line(line);
                    throw e;
                }
            }
            else {
                throw CompilerException(
                    "Expected member selection expression",
                    compiler_errors::STRUCT_MEMBER_SELECTION_ERROR,
                    line
                );
            }

            // s can't be nullptr because any control path leading here will have initialized it
            return *s;
        }
        default:
            throw CompilerException(
                "Illegal expression for procedure object",
                compiler_errors::INVALID_EXPRESSION_TYPE_ERROR,
                line
            );
            break;
    }
}

struct_info &expression_util::get_struct_type(
    const Expression &exp,
    struct_table &structs,
    symbol_table &symbols,
    unsigned int line
) {
    /*

    get_struct_type
    Gets the struct type for the LHS of an expression

    */

    DataType lhs_type = get_expression_data_type(exp, symbols, structs, line);
    if (lhs_type.get_primary() == STRUCT) {
        auto &si = structs.find(lhs_type.get_struct_name(), line);
        return si;
    }
    else {
        throw CompilerException(
            "Expected struct type",
            compiler_errors::STRUCT_TYPE_EXPECTED_ERROR,
            line
        );
    }
}

std::string expression_util::load_into_register(
    symbol& sym,
    reg destination,
    register_usage& context
) {
    /*

    load_into_register
    Loads a given symbol 'sym' into a register 'destination'

    Note this does not work for struct members. This function also assumes the symbol can be put into a register.

    */

    std::stringstream load_ss;
    const std::string reg_string = register_usage::get_register_name(destination, sym.get_data_type());
    const auto reg64_name = register_usage::get_register_name(destination);

    // how we get this data depends on where it lives
    if (sym.get_data_type().get_qualities().is_static()) {
        // static memory can be looked up by name -- variables are in the .bss, .data, or .rodata section
        load_ss << "\t" << "lea " << reg64_name << ", [" << sym.get_name() << "]" << std::endl;
        load_ss << "\t" << "mov " << reg_string << ", [" << reg64_name << "]" << std::endl;
    } 
    else if (sym.get_data_type().get_qualities().is_dynamic()) {
        // dynamic memory
        // since dynamic variables are really just pointers, we need to get the pointer and then dereference it

        // get the dereferenced pointer
        if (
            sym.get_data_type().get_primary() == STRING ||
            sym.get_data_type().get_primary() == ARRAY || 
            sym.get_data_type() == STRUCT || 
            sym.get_data_type().get_primary() == TUPLE
        ) {
            load_ss << "\t" << "mov " << reg_string << ", [rbp - " << sym.get_offset() << "]" << std::endl;
        }
        else {
            load_ss << "\t" << "mov " << reg64_name << ", [rbp - " << sym.get_offset() << "]" << std::endl;
            load_ss << "\t" << "mov " << reg_string << ", [" << reg64_name << "]" << std::endl;
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
            load_ss << "\t" << "mov " << reg_string << ", [rbp - " << sym.get_offset() << "]" << std::endl;
        }
        else {
            auto old_register = sym.get_register();
            load_ss << "\t" << "mov " << reg_string << ", " << register_usage::get_register_name(old_register) << std::endl;
            context.clear(old_register);
        }
    }

    // ensure we mark the symbol as being in this register
    sym.set_register(destination);
    context.set(destination, &sym);

    return load_ss.str();
}
