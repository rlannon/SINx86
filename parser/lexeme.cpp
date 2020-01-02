/*

SIN Toolchain
lexeme.cpp
Copyright 2020 Riley Lannon

*/

#include "lexeme.h"

bool lexeme::operator==(const lexeme& b) {
    // allow lexemes to be compared with the == operator
	// note that we don't care about the line number; we only want to know if they have the same type/value pair
	return ((this->type == b.type) && (this->value == b.value));
}

lexeme::lexeme() {
	this->line_number = 0;	// initialize to 0 by default
}

lexeme::lexeme(std::string type, std::string value, unsigned int line_number) :
    type(type),
    value(value),
    line_number(line_number) 
{
    // body not necessary
}
