/*

SIN Toolchain (x86 target)
type_deduction.cpp
Copyright 2020 Riley Lannon

The implementation of our type deduction functions

*/

#include "type_deduction.h"

Type type_deduction::get_type_from_lexeme(lexeme_type lex_type)
{
	switch (lex_type) {
	case STRING_LEX:
		return STRING;
	case BOOL_LEX:
		return BOOL;
	case INT_LEX:
		return INT;
	case FLOAT_LEX:
		return FLOAT;
	case CHAR_LEX:
		return CHAR;
	default:
		return STRUCT;
	}
}

Type type_deduction::get_type_from_string(std::string candidate) {
	// if it can, this function gets the proper type of an input string
	// an array of the valid types as strings

	// for test our candidate against each item in the array of string_types; if we have a match, return the Type at the same position
	for (size_t i = 0; i < num_types; i++) {
		if (candidate == type_strings[i]) {
			// if we have a match, return it
			return types[i];
		}
		else {
			continue;
		}
	}

	// if we arrive here, we have not found a primitive type, and we should assume it's a struct; if no such struct exists, then the compiler will notice
	return STRUCT;
}

std::string type_deduction::get_string_from_type(Type candidate) {
	// reverse of the above function

	// for test our candidate against each item in the array of string_types; if we have a match, return the string at the same position
	for (size_t i = 0; i < num_types; i++) {
		if (candidate == types[i]) {
			// if we have a match, return it
			return type_strings[i];
		}
		else {
			continue;
		}
	}

	return "struct";
}
