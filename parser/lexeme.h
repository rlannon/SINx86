#pragma once

/*

SIN Toolchain (x86 target)
lexeme.h
Copyright 2020 Riley Lannon

The definition of the struct that contains lexeme data

*/

#include <string>

#include "../util/EnumeratedTypes.h"

struct lexeme {
	lexeme_type type;
	std::string value;
	unsigned int line_number;
	
	// overload the == operator so we can compare two lexemes
	bool operator==(const lexeme& b);

	lexeme();
	lexeme(const lexeme_type type, const std::string& value, const unsigned int line_number);
};
