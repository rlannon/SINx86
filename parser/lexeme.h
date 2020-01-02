#pragma once

/*

SIN Toolchain (x86 target)
lexeme.h
Copyright 2020 Riley Lannon

The definition of the struct that contains lexeme data

*/

#include <string>

struct lexeme {
	std::string type;	// todo: refactor to use enum
	std::string value;
	unsigned int line_number;
	
	// overload the == operator so we can compare two lexemes
	bool operator==(const lexeme& b);

	lexeme();
	lexeme(std::string type, std::string value, unsigned int line_number);
};
