/*

SIN Toolchain (x86 target)
compile_util/utilities.cpp

Some utility functions for the compiler

*/

#include "utilities.h"

// todo: don't use a template here; we can dynamic_cast to symbol because it is the parent class of symbol; similarly, we don't need symbol_table to be a template
DataType get_expression_data_type(std::shared_ptr<Expression> to_eval, symbol_table& symbols, struct_table& structs, unsigned int line) {
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
        case LVALUE:
		case INDEXED:	// since Indexed expressions inherit from LValue, we can use one switch case
        {
            // look into the symbol table for an LValue
            LValue *lvalue = dynamic_cast<LValue*>(to_eval.get());
			std::shared_ptr<symbol> sym;

			try {
				// get the symbol and return its type data
				sym = symbols.find(lvalue->getValue());
			}
			catch (std::exception& e) {
				throw SymbolNotFoundException(line);
			}

			// depending on whether we have an indexed or lvalue expression, we have to return different type data
			if (expression_type == INDEXED) {
				type_information = *sym->get_data_type().get_full_subtype().get();
			}
			else {
				type_information = sym->get_data_type();
			}
            break;
        }
        case LIST:
        {
            // get list type
            ListExpression *init_list = dynamic_cast<ListExpression*>(to_eval.get());
            
            // A list expression is a vector of other expressions, get the first item and pass it into this function recursively
            DataType sub_data_type = get_expression_data_type(init_list->get_list()[0], symbols, structs, line);

            // the subtype will be the current primary type, and the primary type will be array
            sub_data_type.set_subtype(sub_data_type);
            sub_data_type.set_primary(ARRAY);

            // copy it into type_information
            type_information = sub_data_type;
            break;
        }
        case ADDRESS_OF:
		{
			// get the pointer
			AddressOf *addr_of = dynamic_cast<AddressOf*>(to_eval.get());

			// todo: properly implement address-of parsing and evaluation

			break;
		}
        case DEREFERENCED:
        {
            // get the type of the dereferenced pointer
            Dereferenced *deref = dynamic_cast<Dereferenced*>(to_eval.get());
            
            // Dereferenced expressions contain a pointer to another expression; get its type
            type_information = get_expression_data_type(deref->get_contained_expression(), symbols, structs, line);
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
                DataType left = get_expression_data_type(binary->get_left(), symbols, structs, line);
                DataType right = get_expression_data_type(binary->get_right(), symbols, structs, line);

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
            type_information = get_expression_data_type(u->get_operand(), symbols, structs, line);
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
        case SIZE_OF:
            // size_of always returns a const unsigned integer type
            type_information.set_primary(INT);
            type_information.add_qualities(std::vector<SymbolQuality>{CONSTANT, UNSIGNED});
            break;
        case CAST:
        {
            Cast *c = dynamic_cast<Cast*>(to_eval.get());
            if (DataType::is_valid_type(c->get_new_type())) {
                type_information = c->get_new_type();
            }
            else {
                throw CompilerException("Attempt to cast to invalid type", compiler_errors::INVALID_CAST_ERROR, line);
            }
        }
        default:
            throw CompilerException("Invalid expression type", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
            break;
    };

    return type_information;
}

bool is_valid_type_promotion(symbol_qualities left, symbol_qualities right) {
	/*
	
	is_valid_type_promotion
	Ensures that type promotion rules are not broken

	Essentially, the right-hand variability quality must be equal to or *lower* than the left-hand one in the hierarchy

	See doc/Type Compatibility.md for information on type promotion
	
	*/

	if (left.is_const()) {
		return true;	// a const left-hand side will always promote the right-hand expression
	}
	else if (left.is_final()) {
		return !right.is_const();	// a const right-hand argument cannot be demoted
	}
	else {
		return !(right.is_const() || right.is_final());	// the right-hand argument cannot be demoted
	}
}

bool is_valid_cast(DataType &old_type, DataType &new_type) {
    /*

    is_valid_cast
    Determines whether the attempted typecast (from 'old_type' to 'new_type') is valid

    See docs/Typecasting.md for information on typecasting validity rules

    */

    if (old_type.get_primary() == STRING) {
        return false;
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

std::string get_rax_name_variant(DataType t, unsigned int line) {
	/*
	
	get_register_name
	Gets the name of the variant of RAX that is to hold the data of the given type
	
	*/

	std::string reg_string = "";

	if (t.get_width() == sin_widths::BOOL_WIDTH) {
		reg_string = "al";
	}
	else if (t.get_width() == sin_widths::SHORT_WIDTH) {
		reg_string = "ax";
	}
	else if (t.get_width() == sin_widths::INT_WIDTH) {
		reg_string = "eax";
	}
	else if (t.get_width() == sin_widths::PTR_WIDTH) {
		reg_string = "rax";
	}
	else {
		throw CompilerException("Invalid data width for symbol", compiler_errors::DATA_WIDTH_ERROR, line);
	}

	return reg_string;
}

struct_info define_struct(StructDefinition definition) {
    /*
    
    define_struct
    Creates a struct_info object based on a syntax tree for a struct definition

    Since this doesn't actually affect our symbol table, we don't need compiler members
    Note that the caller should ensure that the definition statement occurs within the globa scope -- it is not the responsibility of this function

    @param  definition  The definition statement for the struct
    @return A 'struct_info' object which may be added to the compiler's struct table
    @throws Throws a StructDefinitionException if there are statements other than allocations

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

            // first, ensure that the symbol's type is not this struct
            if ((alloc->get_type_information().get_primary() == STRUCT) && (alloc->get_type_information().get_struct_name() == struct_name)) {
                throw CompilerException(
                    "A struct may not contain an instance of itself; use a pointer instead",
                    compiler_errors::SELF_CONTAINMENT_ERROR,
                    alloc->get_line_number()
                );
            }
            // todo: once references are enabled, disallow those as well -- they can't be null, so that would cause infinite recursion, too

            symbol sym(alloc->get_name(), struct_name, 1, alloc->get_type_information(), current_offset);
            
            // todo: allow default values (alloc-init syntax) in structs
            // to do this, the function might have to be moved out of "utilities" and into "compiler"

            // add that symbol to our vector
            members.push_back(sym);

            // update the data offset
            // todo: handle struct and array members
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
template <typename T>
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
    function_symbol to_return(
        //def.get_name(),
        symbol_table::get_mangled_name(def.get_name()),
        def.get_type_information(),
        formal_parameters,
        def.get_calling_convention()
    );

    // finally, return the function symbol
    return to_return;
}

template symbol generate_symbol(Declaration&, std::string, unsigned int, size_t&);
template symbol generate_symbol(Allocation&, std::string, unsigned int, size_t&);
template <typename T>
symbol generate_symbol(T &allocation, std::string scope_name, unsigned int scope_level, size_t &stack_offset) {
    /*

    generate_symbol
    Creates a symbol for a variable based on its allocation/declaration

    This template function is responsible for creating symbols based on Allocation or Declaration objects. Note that this does not handle the actual allocation of the variable, it just constructs the symbol based on the name, scope, etc. As such, whether the variable was initialized does not matter.
    This will also update the stack offset passed into it to account for the width of the symbol.

    Note this function updates the stack offset *before* it creates the symbol; this prevents us from overwriting the base pointer.
    For example, when an integer is allocated as the first item in a function, its offset will be rbp-4

    @param  allocation  A Declaration or Allocation statement
    @param  scope_name  The name of the scope where the symbol is located
    @param  scope_level The scope level of the symbol
    @param  stack_offset    The stack offset (in bytes) of the symbol from the stack frame base

    @return The constructed symbol object

    */

    DataType &type_info = allocation.get_type_information();

    // adjust the stack offset
    if (type_info.get_primary() == ARRAY && !type_info.get_qualities().is_dynamic()) {
        // non-dynamic array stack offsets have to account for the full array width
        stack_offset += (type_info.get_full_subtype()->get_width() * type_info.get_array_length()) + sin_widths::INT_WIDTH;
    }
    else {
        stack_offset += type_info.get_width();
    }

    symbol to_return(
        //allocation.get_name(),
        symbol_table::get_mangled_name(allocation.get_name()),
        scope_name,
        scope_level,
        type_info,
        stack_offset);

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
        if (((*it != RAX && *it != RBX) || !ignore_ab) && regs.is_in_use(*it)) {
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
        if (((*it != RAX && *it != RBX) || !ignore_ab) && regs.is_in_use(*it)) {
            pop_ss << "\t" << "pop " << register_usage::get_register_name(*it) << std::endl;
        }
    }

    return pop_ss;
}

std::string get_address(symbol &s, reg r) {
    /*

    get_address
    Gets the address of the given symbol in the specified register

    */

    std::string address_info = "";
    std::string reg_name = register_usage::get_register_name(r);

    // if it's static, we can just use the name
    if (s.get_data_type().get_qualities().is_static()) {
        address_info = "\tmov " + reg_name + ", " + s.get_name();
    }
    // otherwise, we need to look in the stack
    else if (s.get_data_type().get_qualities().is_dynamic() || s.get_data_type().get_primary() == STRING) {
        address_info = "\tmov " + reg_name + ", [rbp - " + std::to_string(s.get_offset()) + "]";
    }
    else {
        address_info = "\tmov " + reg_name + ", rbp\n";
        address_info += "\tsub " + reg_name + ", " + std::to_string(s.get_offset()) + "\n";
    }

    return address_info;
}

std::stringstream copy_array(symbol &src, symbol &dest, register_usage &regs) {
    /*

    copy_array
    Calls the SRE function 'sinl_array_copy' to copy array from src to dest

    The SRE parameters are:
        ptr<array> src
        ptr<array> dest
    and the function uses the SINCALL calling convention

    */

	std::stringstream copy_ss;
    
    // push the registers that are in use (subroutine returns void, so we don't need to ignore A and B)
    copy_ss << push_used_registers(regs).str();

    copy_ss << get_address(src, reg::RSI) << std::endl;
    copy_ss << get_address(dest, reg::RDI) << std::endl;
    copy_ss << "\t" << "mov ecx, " << src.get_data_type().get_full_subtype()->get_width() << std::endl;
    copy_ss << "\t" << "push rbp" << std::endl;
    copy_ss << "\t" << "mov rbp, rsp" << std::endl;
    copy_ss << "\t" << "call sinl_array_copy" << std::endl;
    copy_ss << "\t" << "mov rsp, rbp" << std::endl;
    copy_ss << "\t" << "pop rbp" << std::endl;

    // restore registers
    copy_ss << pop_used_registers(regs).str();
    
	return copy_ss;
}

std::stringstream copy_string(symbol &src, symbol &dest, register_usage &regs) {
    /*

    copy_string
    Calls the SRE function to copy one string to another

    The SRE parameters are:
        ptr<string> src
        ptr<string> dest
    It returns the address of the new destination string in RAX

    */

    std::stringstream copy_ss;

    // preserve registers in use, ignoring RAX and RBX
    copy_ss << push_used_registers(regs, true).str();

    // get the pointers
    copy_ss << get_address(src, RSI) << std::endl;
    copy_ss << get_address(dest, RDI) << std::endl;
    copy_ss << "\t" << "call sinl_string_copy" << std::endl;
    copy_ss << get_address(dest, RBX) << std::endl;
    copy_ss << "\t" << "mov [rbx], rax" << std::endl;

    // restore registers
    copy_ss << pop_used_registers(regs, true).str();

    return copy_ss;
}
