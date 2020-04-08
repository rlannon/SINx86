/*

SIN Toolchain (x86 target)
dot_and_arrow.cpp
Copyright 2020 Riley Lannon

Handles expressions which use the dot and arrow operators

*/

#include "compiler.h"

std::stringstream compiler::evaluate_arrow(Binary &arrow_exp, unsigned int line) {
	/*
	
	evaluate_arrow
	Generates code to evaluate an expression that uses the arrow operator (e.g., "p->a")
	
	*/
}

std::stringstream compiler::evaluate_dot(Binary &dot_exp, unsigned int line) {
	/*

	evaluate_dot
	Generates code to evaluate an expression that uses the dot operator (e.g., "p.a")

	The left-hand expression must be "struct" type in order for the expression to be valid.

	*/

	// ensure left_type is of 'struct' data type
	DataType &left_type = get_expression_data_type(dot_exp.get_left(), this->symbols, line);
	if (left_type.get_primary() == STRUCT) {
		DataType &right_type = get_expression_data_type(dot_exp.get_right(), this->symbols, line);
	}
	else {
		
	}
}
