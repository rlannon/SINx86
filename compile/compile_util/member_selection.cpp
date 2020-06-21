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
	if (exp.get_operator() != DOT) {
		throw CompilerException("Expected dot operator in member selection", compiler_errors::OPERATOR_TYPE_ERROR, line);
	}

	member_selection m;

	// todo: allow indexed expressions on LHS

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
	else {
		throw CompilerException("Invalid expression in left-hand position of member selection", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
	}

	// note a unary expression should never appear on the left hand side, as they all have lower precedences than the dot operator

	// get the struct_info object for the last struct in the chain; this is where we look up the next symbol
	if (!structs.contains(m.last().get_data_type().get_struct_name())) {
		throw SymbolNotFoundException(line);
	}
	struct_info& last_struct = structs.find(m.last().get_data_type().get_struct_name());

	// todo: allow indexed expressions on RHS

	// now, handle the right hand side -- note that dereferenced expressions are forbidden here (only allowed on the left side)
	if (exp.get_right()->get_expression_type() == LVALUE) {
		LValue* right = dynamic_cast<LValue*>(exp.get_right().get());
		
		// get the symbol and append it
		try {
			symbol& right_sym = last_struct.get_member(right->getValue());
			m.append(right_sym);
		}
		catch (std::exception& e) {
			throw SymbolNotFoundException(line);
		}
	}
	else if (exp.get_right()->get_expression_type() == UNARY) {
		throw CompilerException("Unary expressions are not allowed on the right-hand side of a member selection expression (e.g., a.*b or a.-b); proper syntax is *a.b and -a.b, respectively", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
	}
	else {
		throw CompilerException("Invalid expression type on right-hand side of member selection", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
	}

	// issue a warning if the first member is marked as freed (we can't know for sure -- it may not be freed in the scope we are looking in, so issue a warning, not an error)
	if (m.first().was_freed()) {
		compiler_warning(
			"Symbol '" + m.first().get_name() + "' may have been freed",
			compiler_errors::DATA_FREED,
			line
		);
	}

	return m;
}

void member_selection::append(symbol & to_add)
{
	// appends an element to the list
	this->symbols.push_back(to_add);

	// if the iterator points to the end, then we must update it to point it to the last element
	if (this->it == this->symbols.end()) {
		this->it--;
	}
}

symbol & member_selection::last()
{
	// sets the iterator to the last element and returns it
	if (this->symbols.empty()) {
		throw std::out_of_range("Empty list in member selection object");
	}

	this->it = this->symbols.end();
	this->it--;
	return *this->it;
}

symbol & member_selection::first()
{
	// sets the iterator to the first element and returns it
	if (this->symbols.empty()) {
		throw std::out_of_range("Empty list in member selection object");
	}

	this->it = this->symbols.begin();
	return *this->it;
}

symbol & member_selection::peek_previous()
{
	// peeks the member *before* the current one
	if (this->it == this->symbols.begin()) {
		throw std::out_of_range("Cannot peek before first element in list");
	}

	std::list<symbol>::iterator temp = this->it;
	--temp;
	return *temp;
}

symbol & member_selection::previous()
{
	// decrements the iterator and returns the result
	if (this->it == this->symbols.begin()) {
		throw std::out_of_range("Cannot peek before the first element in list");
	}

	this->it--;
	return *it;
}

symbol & member_selection::peek() {
	// if the iterator isn't at the end, get the next item
	std::list<symbol>::iterator temp = this->it;
	temp++;
	if (temp == this->symbols.end()) {
		throw std::out_of_range("No more members in member selection list");
	}

	return *temp;
}

symbol & member_selection::next() {
	// increment the iterator and return the value at it
	this->it++;
	if (this->it == this->symbols.end()) {
		throw std::out_of_range("No more members in member selection list");
	}

	return *this->it;
}

bool member_selection::is_at_end() {
	// check if we are pointing to the last element
	std::list<symbol>::iterator temp = it;
	++temp;
	return (temp == this->symbols.end());
}

member_selection & member_selection::operator=(member_selection right) {
	/*
	
	operator==
	The overloaded assignment operator, allowing us to initialize member_selection objects with other such objects properly
	
	@param	right	A reference to the right hand member_selection object

	*/

	this->symbols = right.symbols;	// all we really need to do is initialize this list with the other one
	this->it = this->symbols.begin();
	return *this;
}

member_selection::member_selection(Binary & exp, struct_table & structs, symbol_table & symbols, unsigned int line) : member_selection()
{
	*this = member_selection::create_member_selection(exp, structs, symbols, line);
	this->it = this->symbols.begin();
}

member_selection::member_selection()
{
	this->it = this->symbols.begin();
}

member_selection::~member_selection()
{
}
