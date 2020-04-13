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
			A.	Ensure it is a correct type (LValue only)
			B.	Look up the symbol within the left-hand struct
			C.	Create a node pointing to this symbol
		III. Finalize our member_selection object and return it

	If the expression we encounter is a Dereferenced expression, we need to get the symbol being dereferenced correctly. Further, the compiler also needs to verify that the number of dereferences is actually accessing the right object -- otherwise, we will run into problems.

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

	// first, handle the left hand side
	if (exp.get_left()->get_expression_type() == BINARY) {
		Binary* left = dynamic_cast<Binary*>(exp.get_left().get());
		m = create_member_selection(*left, structs, symbols, line);
	}
	else if (exp.get_left()->get_expression_type() == LVALUE) {
		LValue* left = dynamic_cast<LValue*>(exp.get_left().get());

		// get the symbol information
		symbol* left_sym = dynamic_cast<symbol*>(symbols.find(left->getValue()).get());
		if (left_sym->get_data_type().get_primary() != STRUCT) {
			throw CompilerException("Expected left-hand argument of 'struct' type", compiler_errors::STRUCT_TYPE_EXPECTED_RROR, line);
		}

		// add the symbol to our list
		m.append(*left_sym);
	}
	else if (exp.get_left()->get_expression_type() == DEREFERENCED) {
		Dereferenced* left = dynamic_cast<Dereferenced*>(exp.get_left().get());
		// todo: handle this such that pointers to pointers, etc, are dereferenced correctly
	}
	else {
		throw CompilerException("Invalid expression in left-hand position of member selection", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
	}

	// now, handle the right hand side -- note that dereferenced expressions are forbidden here (only allowed on the left side)
	if (exp.get_right()->get_expression_type() == LVALUE) {
		LValue* right = dynamic_cast<LValue*>(exp.get_right().get());
		
		// get the symbol and append it
		try {
			symbol* right_sym = dynamic_cast<symbol*>(symbols.find(right->getValue()).get());
			m.append(*right_sym);
		}
		catch (std::exception& e) {
			throw SymbolNotFoundException(line);
		}
	}
	else {
		throw CompilerException("Invalid expression type in right-hand position of member selection", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
	}

	return m;
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
