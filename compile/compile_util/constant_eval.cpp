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

std::string compile_time_evaluator::evaluate_expression(std::shared_ptr<Expression> to_evaluate, std::string scope_name, unsigned int scope_level)
{
	/*
	
	evaluate_expression
	Evaluates a constexpr
	
	*/
	
	return std::string();
}

compile_time_evaluator::compile_time_evaluator()
{
}

compile_time_evaluator::~compile_time_evaluator()
{
}
