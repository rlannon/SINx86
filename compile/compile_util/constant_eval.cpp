/*

SIN Toolchain (x86 target)
constant_eval.cpp
Copyright 2020 Riley Lannon

The implementation of our compile-time evaluation functionality

*/

#include "constant_eval.h"

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

void compile_time_evaluator::add_constant(Allocation & alloc, symbol & s)
{
	/*
	
	add_constant
	Adds a constant to the table according to its allocation

	This function will take the symbol allocated by the compiler and use it to initialize the symbol in the constant table
	It will also evaluate the initial value using the 'evaluate_expression' method and store it in the class as a string
	
	*/
}

const_symbol compile_time_evaluator::lookup(std::string sym_name, std::string scope_name, unsigned int scope_level, unsigned int line) const {
	/*
	
	lookup
	Returns the symbol with the specified name in the specified scope

	This function utilizes a function to create a mangled version of the symbol name based on its scope. See compile_time_evaluator::get_mangled_name for details on how it works.

	@param	sym_name	The name of the symbol
	@param	scope_name	The name of the scope to look in
	@param	scope_level	The level of that scope to look in
	@param	line	The line where the constexpr evaluation occurs

	@returns	A const_symbol object from the table, or an empty one if none is found

	@throws	This function throws a SymbolNotFoundException if the symbol in question cannot be located
	
	*/

	const_symbol to_return;

	std::unordered_map<std::string, const_symbol>::iterator it = this->constants.find(
		compile_time_evaluator::get_mangled_name(sym_name, scope_name, scope_level)
	);

	if (it == this->constants.end()) {
		throw SymbolNotFoundException(line);
	}
	else {
		to_return = it->second;
	}

	return to_return;
}

void compile_time_evaluator::remove_symbols_in_scope(std::string scope_name, unsigned int scope_level)
{
	/*
	
	remove_symbols_in_scope
	Removes all symbols that are located in the given scope from the table
	
	*/

	std::unordered_map<std::string, const_symbol>::iterator it = this->constants.begin();
	while (it != this->constants.end()) {
		if (it->second.get_scope_name() == scope_name && it->second.get_scope_level() == scope_level) {
			it = this->constants.erase(it);
		}
		else {
			it++;
		}
	}
}

std::string compile_time_evaluator::evaluate_literal(Literal & exp)
{
	/*
	
	evaluate_literal
	Evaluates a literal expression

	*/

	return exp.get_value();
}

std::string compile_time_evaluator::evaluate_lvalue(LValue & exp, std::string scope_name, unsigned int scope_level, unsigned int line)
{
	/*
	
	evaluate_lvalue
	Evaluates a const lvalue expression (a const symbol)
	
	*/

	const_symbol to_return = this->lookup(exp.getValue(), scope_name, scope_level, );
	return to_return.get_value();
}

std::string compile_time_evaluator::evaluate_expression(std::shared_ptr<Expression> to_evaluate, std::string scope_name, unsigned int scope_level, unsigned int line)
{
	/*
	
	evaluate_expression
	Evaluates a constexpr

	This function will dispatch appropriately and return the final result

	@param	to_evaluate	The expression we wish to evaluate
	@param	scope_name	The name of the scope where the expression occurs (for variable selection)
	@param	scope_level	The scope block number (depth) where the expression occurs (again for variable selection)
	
	*/

	std::string evaluated_expression;

	if (to_evaluate->get_expression_type() == LITERAL) {
		Literal exp = *dynamic_cast<Literal*>(to_evaluate.get());
		evaluated_expression = compile_time_evaluator::evaluate_literal(exp);
	}
	else if (to_evaluate->get_expression_type() == LVALUE) {
		LValue lvalue = *dynamic_cast<LValue*>(to_evaluate.get());
		evaluated_expression = this->evaluate_lvalue(lvalue, scope_name, scope_level, line);
	}
	// todo: more expression types
	else {
		// throw an exception as the expression was invalid
		throw CompilerException("Could not evaluate compile-time constant; invalid expression type", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
	}
	
	return evaluated_expression;
}

compile_time_evaluator::compile_time_evaluator()
{
}

compile_time_evaluator::~compile_time_evaluator()
{
}
