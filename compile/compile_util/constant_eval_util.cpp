/*

SIN Toolchain (x86 target)
constant_eval_util.cpp
Copyright 2020 Riley Lannon

The implementations of various utility functions for the compile_time_evaluator class

*/

#include "constant_eval.h"

void compile_time_evaluator::add_constant(const Allocation & alloc, const symbol & s)
{
	/*

	add_constant
	Adds a constant to the table according to its allocation

	This function will take the symbol allocated by the compiler and use it to initialize the symbol in the constant table
	It will also evaluate the initial value using the 'evaluate_expression' method and store it in the class as a string

	@param	alloc	A reference to the allocation statement that allocates the constant in question
	@param	s	The symbol generated by the allocation statement

	*/

	// get the symbol
    if (alloc.get_initial_value()) {
        std::string initial_value = this->evaluate_expression(*alloc.get_initial_value(), s.get_scope_name(), s.get_scope_level(), alloc.get_line_number());
        const_symbol sym(s, initial_value);	// initialize our const_symbol object

        this->constants->insert(std::make_shared<const_symbol>(sym));	// todo: ensure the symbol was inserted successfully
    }
    else {
        throw ConstInitializationException(alloc.get_line_number());
    }
}

const const_symbol& compile_time_evaluator::lookup(const std::string& sym_name, const std::string& scope_name, unsigned int scope_level, unsigned int line) const {
	/*

	lookup
	Returns the symbol with the specified name in the specified scope

	@param	sym_name	The name of the symbol
	@param	scope_name	The name of the scope to look in
	@param	scope_level	The level of that scope to look in
	@param	line	The line where the constexpr evaluation occurs

	@returns	A const_symbol object from the table, or an empty one if none is found

	@throws	This function throws a SymbolNotFoundException if the symbol in question cannot be located

	*/

	const_symbol to_return;

	try {
		return dynamic_cast<const const_symbol&>(this->constants->find(sym_name));
	}
	catch (SymbolNotFoundException & e) {
		e.set_line(line);
        throw e;
	}
    catch (std::bad_cast &e) {
        throw CompilerException(
            "Expected a const symbol",
            compiler_errors::NON_CONST_VALUE_ERROR,
            line
        );
    }
}

void compile_time_evaluator::leave_scope(const std::string& name, unsigned int level)
{
	/*

	remove_symbols_in_scope
	Removes all symbols that are located in the given scope from the table

	*/

	this->constants->leave_scope(name, level);
}
