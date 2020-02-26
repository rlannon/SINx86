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
            sub_data_type.set_subtype(sub_data_type);
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

bool returns(StatementBlock &to_check) {
	// Checks whether a given function definition will return a value
	// todo: refactor this entire function to work more efficiently


	if (to_check.has_return) {
		return true;
	}
	else {
		bool r = false;

		// iterate through statements to see if we have an if/else block; if so, check *those* for return values
		for (std::shared_ptr<Statement> s : to_check.statements_list) {

			// todo: refactor interior of the loop

			if (s->get_statement_type() == stmt_type::IF_THEN_ELSE) {
				IfThenElse *ite = dynamic_cast<IfThenElse*>(s.get());
				StatementBlock &if_branch = *ite->get_if_branch().get();

				if (ite->get_else_branch()) {
					StatementBlock &else_branch = *ite->get_else_branch().get();

					r = returns(if_branch) && returns(else_branch);
				}
				else {
					r = false;
				}
			}

			if (!r) {
				return false;
			}

			// todo: convert to a while loop
		}
	}
}

bool can_pass_in_register(DataType to_check) {
    // Checks whether the given DataType can be passed in a register or if it must be passed on the stack

    bool can_pass = false;

    Type primary = to_check.get_primary();
    if (primary == ARRAY || primary == STRUCT || primary == STRING) {
        // if the type is dyamic, then we can -- we are really passing in a pointer into the function
        // todo: can we have static parameters?
        can_pass = to_check.get_qualities().is_dynamic();
    } else {
        // if we have numeric, bool, or char types, then we can pass on a register
        can_pass = true;
    }

    return can_pass;
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
    std::string struct_name = definition.get_name();

    // iterate through our definition statements and create symbols for all struct members
    std::vector<symbol> members;
    size_t current_offset = 0;
    for (std::shared_ptr<Statement> s: definition.get_procedure()->statements_list) {
        // Only allocations are allowed within a struct body
        if (s->get_statement_type() == ALLOCATION) {
            // cast to Allocation and create a symbol
            Allocation *alloc = dynamic_cast<Allocation*>(s.get());
            symbol sym(alloc->get_name(), struct_name, 1, alloc->get_type_information(), current_offset);
            
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

// Since the declaration and implementation are in separate files, we need to say which types may be used with our template functions
template function_symbol create_function_symbol(FunctionDefinition);
template function_symbol create_function_symbol(Declaration);

template <class T>
function_symbol create_function_symbol(T def) {
    /*

    create_function_symbol
    Creates a symbol for a function based on either a definition or a declaration

    This function is responsible for turning the Statement objects containing parameters into symbol objects, but it _does not_ add them to the symbol table (as it is not a member of compiler)

    @param  def The definition or declaration from which to create our symbol
    @return A symbol containing the function signature

    */

    std::string scope_name = def.get_name();
    unsigned int scope_level = 1;
    size_t stack_offset = 0;

    // construct our formal parameters
    std::vector<symbol> formal_parameters;

    // now, determine which registers can hold which parameters
    for (std::shared_ptr<Statement> param: def.get_formal_parameters()) {
        // create the symbol based on our statement
        symbol param_sym;

        // cast to the appropriate symbol type
        if (param->get_statement_type() == DECLARATION) {
            Declaration *param_decl = dynamic_cast<Declaration*>(param.get());
            param_sym = generate_symbol(*param_decl, scope_name, scope_level, stack_offset);
        } else if (param->get_statement_type() == ALLOCATION) {
            Allocation *param_alloc = dynamic_cast<Allocation*>(param.get());
            param_sym = generate_symbol(*param_alloc, scope_name, scope_level, stack_offset);
        } else {
            // todo: remove? these errors should be caught by the parser
            throw CompilerException("Invalid statement type in function signature", compiler_errors::ILLEGAL_OPERATION_ERROR, def.get_line_number());
        }

        formal_parameters.push_back(param_sym);
    }

    // construct the object
    function_symbol to_return(def.get_name(), def.get_type_information(), formal_parameters, def.get_calling_convention());

    // finally, return the function symbol
    return to_return;
}

template symbol generate_symbol(Declaration&, std::string, unsigned int, size_t&);
template symbol generate_symbol(Allocation&, std::string, unsigned int, size_t&);

template <class T>
symbol generate_symbol(T &allocation, std::string scope_name, unsigned int scope_level, size_t &stack_offset) {
    /*

    generate_symbol
    Creates a symbol for a variable based on its allocation/declaration

    This template function is responsible for creating symbols based on Allocation or Declaration objects. Note that this does not handle the actual allocation of the variable, it just constructs the symbol based on the name, scope, etc. As such, whether the variable was initialized does not matter.
    This will also update the stack offset passed into it to account for the width of the symbol.

    @param  allocation  A Declaration or Allocation statement
    @param  scope_name  The name of the scope where the symbol is located
    @param  scope_level The scope level of the symbol
    @param  stack_offset    The stack offset (in bytes) of the symbol from the stack frame base

    @return The constructed symbol object

    */

    // construct the symbol
    symbol to_return(allocation.get_name(), scope_name, scope_level, allocation.get_type_information(), stack_offset);
    stack_offset += allocation.get_type_information().get_width();  // update the stack offset

    return to_return;
}

std::stringstream push_used_registers(register_usage regs, bool ignore_ab) {
    /*

    push_used_registers
    Given a register_usage object, push all registers that are *currently* in use

    @param  regs    The object containing which registers are in use
    @param  ignore_ab   Whether we should ignore RAX and RBX when we push the registers; defaults to false

    */

    std::stringstream push_ss;

    for (
        std::vector<reg>::const_iterator it = register_usage::all_regs.begin();
        it != register_usage::all_regs.end();
        it++
    ) {
        if ((*it != RAX && *it != RBX) || !ignore_ab) {
            push_ss << "\t" << "push " << register_usage::get_register_name(*it) << std::endl;
        }
    }

    return push_ss;
}

std::stringstream pop_used_registers(register_usage regs, bool ignore_ab) {
    /*

    pop_used_registers
    Pops all registers marked as 'in use' in a register_usage object

    Uses a reverse_iterator to iterate through all_regs in order to find registers

    @param  regs    The registers that are in use that must be restored
    @param  ignore_ab   Whether we ignored RAX and RBX in the push; defaults to false

    */

    std::stringstream pop_ss;

    for (
        std::vector<reg>::const_reverse_iterator it = register_usage::all_regs.rbegin();
        it != register_usage::all_regs.rend();
        it++
    ) {
        if ((*it != RAX && *it != RBX) || !ignore_ab) {
            pop_ss << "\t" << "pop " << register_usage::get_register_name(*it) << std::endl;
        }
    }

    return pop_ss;
}

std::stringstream copy_array(DataType array_type) {
	std::stringstream copy_ss;

	return copy_ss;
}
