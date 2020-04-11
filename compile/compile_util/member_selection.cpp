/*

SIN Toolchain (x86 target)
member_selection.cpp
Copyright 2020 Riley Lannon

Implementation of the member_selection class

*/

#include "member_selection.h"

member_selection member_selection::create_member_selection(Binary &exp, struct_table& structs, symbol_table& symbols, unsigned int line) {
	/*
	
	create_member_selection
	Creates a member_selection based on a binary expression (operator must be dot or arrow)
	
	The algorithm works, generally, as follows:
		I. Look at the left side
			A.	If the type is 'binary', we need to call this function recursively on it
			B.	If the type is some other valid expression (LValue or Dereferenced) then we look up the information for that struct
		II. Look at the right side
			A.	Ensure it is a correct type (LValue only -- dereferences must use the arrow operator)
			B.	Look up the symbol within the left-hand struct
			C.	Create a node pointing to this symbol
		III. Finalize our member_selection object and return it

	@param	exp	The binary dot/arrow expression we are evaluating
	@param	structs	The struct table we need to look into for our struct data
	@param	symbols	The symbol table we need to look into for our symbols
	@param	line	The line number where this expression appears
	
	@returns	A member_selection object containing the data we need

	@throws	A specialized CompilerException if there are any errors

	*/

	// ensure the operator is valid
	if (exp.get_operator() != DOT && exp.get_operator() != ARROW) {
		throw CompilerException("Expected dot or arrow operator in member selection", compiler_errors::OPERATOR_TYPE_ERROR, line);
	}

	member_selection m;
	if (exp.get_left()->get_expression_type() == BINARY) {
		Binary* left = dynamic_cast<Binary*>(exp.get_left().get());
		m = create_member_selection(*left, structs, symbols, line);
	}
	else if (exp.get_left()->get_expression_type() == LVALUE) {
		// todo: handle left-hand lvalue expressions
	}
	else if (exp.get_left()->get_expression_type() == DEREFERENCED) {
		// todo: handle dereferenced left-hand expression
	}

	return member_selection();
}

void member_selection::append(symbol & to_add)
{
	this->symbols.push_back(to_add);
}

symbol & member_selection::last()
{
	return this->symbols.back();
}

symbol & member_selection::first()
{
	return this->symbols.front();
}

member_selection member_selection::operator=(member_selection& right) {
	/*
	
	operator==
	The overloaded assignment operator, allowing us to initialize member_selection objects with other such objects properly
	
	@param	right	A reference to the right hand member_selection object

	*/

	this->symbols = right.symbols;	// all we really need to do is initialize this list with the other one
	return *this;
}

member_selection::member_selection()
{
}

member_selection::~member_selection()
{
}
