/*

SIN Toolchain (x86 target)
constant_eval.cpp
Copyright 2020 Riley Lannon

The implementation of our compile-time evaluation functionality

*/

#include "constant_eval.h"

void compile_time_evaluator::add_constant(Allocation & alloc, symbol & s)
{
	/*
	
	add_constant
	Adds a constant to the table according to its allocation

	This function will take the symbol allocated by the compiler and use it to initialize the symbol in the constant table
	It will also evaluate the initial value using the 'evaluate_expression' method and store it in the class as a string
	
	*/
}

const_symbol compile_time_evaluator::lookup(std::string sym_name, std::string scope_name, unsigned int scope_level) {
	/*
	
	lookup
	Returns the symbol with the specified name in the specified scope

	This function utilizes a function to create a mangled version of the symbol name based on its scope. See compile_time_evaluator::get_mangled_name for details on how it works.
	
	*/

	return const_symbol(symbol(), std::string());
}

std::string compile_time_evaluator::get_mangled_name(std::string sym_name, std::string scope_name, unsigned int scope_level)
{
	/*
	
	get_mangled_name
	Returns the mangled name of the specified symbol

	Because this evaluator needs to track constants in multiple scopes, and since those variables might have the same name (just in different scopes), it needs to ensure it generates a unique name for each symbol added and that it obtains the proper symbol in the lookup process
	
	The mangling process works as follows:
		- the name of the scope
		- an underscore
		- the scope's level
		- an underscore
		- the name of the symbol
	So, for example, a symbol called 'i' located within an 'if' block in the function 'main' would get the following symbol:
		main_2_i
	And a variable called 'main_2_i' in the global scope would get:
		global_0_main_2_i
	
	@param	sym_name	The name of the symbol
	@param	scope_name	The name of the scope where the symbol appears
	@param	scope_level	The scope block level/depth where the symbol appears in its scope

	@return	The mangled name of the symbol

	*/
	
	return scope_name + "_" + std::to_string(scope_level) + "_" + sym_name;
}

std::string compile_time_evaluator::evaluate_expression(std::shared_ptr<Expression> to_evaluate, std::string scope_name, unsigned int scope_level)
{
	/*
	
	evaluate_expression
	Evaluates a constexpr

	This function will dispatch appropriately and return the final result

	@param	to_evaluate	The expression we wish to evaluate
	@param	scope_name	The name of the scope where the expression occurs (for variable selection)
	@param	scope_level	The scope block number (depth) where the expression occurs (again for variable selection)
	
	*/

	if (to_evaluate->get_expression_type() == LITERAL) {

	}
	
	return std::string();
}

compile_time_evaluator::compile_time_evaluator()
{
}

compile_time_evaluator::~compile_time_evaluator()
{
}
