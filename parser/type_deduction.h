/*

SIN Toolchain (x86 target)
type_deduction.h
Copyright 2020 Riley Lannon

A simple parser utility for deducing types from strings, etc.

*/

// todo: expand for other operator parsing utilities, etc.?

#pragma once

#include "../util/EnumeratedTypes.h"
#include <cinttypes>
#include <string>

namespace type_deduction {
    // data members
	const size_t num_types = 10;
	const std::string type_strings[10]{ "char", "int", "float", "string", "bool", "void", "ptr", "raw", "array", "struct" };
    const Type types[10]{ CHAR, INT, FLOAT, STRING, BOOL, VOID, PTR, RAW, ARRAY, STRUCT };

    // functions
	Type get_type_from_lexeme(lexeme_type lex_type);
	Type get_type_from_string(std::string candidate);
    std::string get_string_from_type(Type candidate);
}
