/*

SIN Toolchain
EnumeratedTypes.h
Copyright 2019 Riley Lannon

The purpose of this file is to have all of the enumerated types defined in a single place so they can be referred to be multiple files without creating any circular dependencies.
These are to be used so that this code might be more maintainable and less error-prone; while all of these could be replaced with std::string, using an enum centralizes the definitions and makes it much more difficult to have an error hidden somewhere in the code because you used "Dynamic" instead of "dynamic" as a symbol quality, for example. Further, using enumerated types also makes it more clear in the code what is what instead of having set codes for statement types or symbol qualities. This way, it is very clear what the type or quality is without needing to look up anything else.

*/

#pragma once

enum lexeme_type {
	// The types of lexemes possible in the lexer, expressed as an enum
	FLOAT_LEX,
	INT_LEX,
	STRING_LEX,
	CHAR_LEX,
	BOOL_LEX,
	IDENTIFIER_LEX,
	KEYWORD_LEX,
	PUNCTUATION,
	OPERATOR,
	ESCAPED,
	NULL_LEXEME
};

enum stmt_type {
	// The various types of statements we can have in SIN
	STATEMENT_GENERAL,
	INCLUDE,
	DECLARATION,
	ALLOCATION,
	ASSIGNMENT,
	MOVEMENT,
	RETURN_STATEMENT,
	IF_THEN_ELSE,
	WHILE_LOOP,
	FUNCTION_DEFINITION,
	STRUCT_DEFINITION,
	CALL,
	INLINE_ASM,
	FREE_MEMORY,
	SCOPE_BLOCK,
	CONSTRUCTION_STATEMENT,
	COMPOUND_ASSIGNMENT
};


enum exp_type {
	// So that we can list all of the various expression types in one place
	EXPRESSION_GENERAL,
	LITERAL,	// can be a literal int (e.g., 5), a literal float (e.g., 1.2), a literal string (e.g., "hello"), a literal bool (true/false) ...
	IDENTIFIER,		// any named data
	LIST,	// initializer-lists, mostly; syntax is { ... , ... }
	INDEXED,
	BINARY,
	UNARY,
	CALL_EXP,
	CAST,
	ATTRIBUTE,
	KEYWORD_EXP,
	CONSTRUCTION_EXP,
    PROC_EXP
};

enum attribute {
	NO_ATTRIBUTE,
	LENGTH,
	SIZE,
	VARIABILITY
};

enum SymbolType {
	// So that we know whether a symbol is a variable, function definition, struct definition...
	VARIABLE,
	FUNCTION_SYMBOL,
	STRUCT_SYMBOL
};

// todo: change to unordered_map?
enum SymbolQuality {
	// So that the symbol's quality does not need to be stored as a string
	NO_QUALITY,
	CONSTANT,
	FINAL,
	STATIC,
	DYNAMIC,
	SIGNED,
	UNSIGNED,
	LONG,
	SHORT,
	SINCALL_CONVENTION,
	C64_CONVENTION,
	WINDOWS_CONVENTION,
	EXTERN,
    UNMANAGED
};


enum exp_operator {
	// So that we have a clear list of operators
	COPY_ASSIGN,	// the = operator
	RIGHT_ARROW,	// the -> operator
	LEFT_ARROW,	// the <- operator
	PLUS_EQUAL,	// +=
	MINUS_EQUAL,	// -=
	MULT_EQUAL,	// *=
	DIV_EQUAL,	// /=
	MOD_EQUAL,	// %=
	AND_EQUAL,	// &=
	OR_EQUAL,	// |=
	XOR_EQUAL,	// ^=
	LEFT_SHIFT_EQUAL,	// <<=
	RIGHT_SHIFT_EQUAL,	// >>=
	PLUS,
	MINUS,
	UNARY_PLUS,
	UNARY_MINUS,
	MULT,
	DIV,
	EQUAL,
	NOT_EQUAL,
	GREATER,
	LESS,
	GREATER_OR_EQUAL,
	LESS_OR_EQUAL,
	AND,	// 'AND' is logical-AND (C++ &&) -- Python keyword 'and'
	NOT,	// logical not
	OR,		// logical or
	XOR,	// logical xor
	MODULO,
	BIT_AND,	// 'BIT_AND' is bitwise-AND (C++ &) -- operator '&'
	BIT_OR,		// same goes with BIT_OR
	BIT_XOR,	// same with BIT_XOR
	BIT_NOT,	// same with BIT_NOT
	LEFT_SHIFT,
	RIGHT_SHIFT,
	DOT,	// the . operator
	INDEX,	// square brackets for indexing
	ADDRESS,		// $
	DEREFERENCE,	// *
	TYPECAST,	// the 'as' operator
	ATTRIBUTE_SELECTION,	// :
	CONTROL_TRANSFER,	// @, used in function calls
    PROC_OPERATOR,  // (), marks a procedure
	SCOPE_RESOLUTION,	//	::
	NO_OP
};


enum Type {
	// So that our types are all clearly defined
	NONE,
    CHAR,
	INT,
	FLOAT,
	STRING,
	BOOL,
	VOID,
	PTR,
	REFERENCE,
	RAW,
	ARRAY,
	STRUCT,
	TUPLE
};

enum reg {
	// The registers available to us

	// It's possible we won't have any registers available
	NO_REGISTER = 0,

    // Integer registers
    RAX,
    RBX,
    RCX,
    RDX,
    RSI,
    RDI,

	// x64 extension
    R8,
    R9,
    R10,
    R11,
    R12,
    R13,
    R14,
    R15,

	// 128-bit SSE registers
	XMM0,
	XMM1,
	XMM2,
	XMM3,
	XMM4,
	XMM5,
	XMM6,
	XMM7
};


enum calling_convention {
	// Calling conventions for functions
	SINCALL,
	SYSTEM_V,
	WIN_64
};
