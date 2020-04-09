/*

SIN Toolchain (x86 target)
dot_and_arrow.cpp
Copyright 2020 Riley Lannon

Handles expressions which use the dot and arrow operators

*/

#include "compiler.h"

// todo: combine dot and arrow into one function

std::stringstream compiler::evaluate_arrow(Binary &arrow_exp, unsigned int line) {
	/*
	
	evaluate_arrow
	Generates code to evaluate an expression that uses the arrow operator (e.g., "p->a")
	
	*/

	std::stringstream eval_ss;

	if (member_selection_types_valid(arrow_exp, this->symbols, line)) {
		// todo: evaluate arrows
	}
	else {
		throw CompilerException(
			"Invalid left-hand data type in arrow operator expression; 'struct' expected",
			compiler_errors::STRUCT_TYPE_EXPECTED_RROR,
			line
		);
	}

	return eval_ss;
}

std::stringstream compiler::evaluate_dot(Binary &dot_exp, unsigned int line) {
	/*

	evaluate_dot
	Generates code to evaluate an expression that uses the dot operator (e.g., "p.a")

	The left-hand expression must be "struct" type in order for the expression to be valid. It can really only be one of the following expressions:
		- lvalue
		- dereferenced
		- binary (dot/arrow expression)
	The compiler evaluates the address of the left-hand struct and uses the 

	*/

	std::stringstream eval_ss;

	// ensure left_type is of 'struct' data type
	if (member_selection_types_valid(dot_exp, this->symbols, line)) {
		
	}
	else {
		throw CompilerException(
			"Invalid left-hand data type in dot operator expression; 'struct' expected",
			compiler_errors::STRUCT_TYPE_EXPECTED_RROR,
			line
		);
	}

	return eval_ss;
}
