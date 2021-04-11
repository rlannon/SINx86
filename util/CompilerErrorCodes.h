/*

SIN Toolchain (x86 target)
CompilerErrorCodes.h

Some error code constants for the compiler error handler

Codes fall into the following categories:
    000 -   099:    Illegal operations
        Used for things like assigning to a const qualified variable
    100 -   199:    Location/undefined errors
        Used when something couldn't be found or resolved
    200 -   299:    Type errors
        Used for things like type mismatches
    300 -   399:    Internal errors

*/

#pragma once

namespace compiler_errors {
    // Illegal operations
    const unsigned int UNSUPPORTED_ERROR = 0;
    const unsigned int CONST_ASSIGNMENT_ERROR = 1;
    const unsigned int FINAL_ASSIGNMENT_ERROR = 2;
    const unsigned int DATA_WIDTH_ERROR = 3;
    const unsigned int OUT_OF_BOUNDS = 4;
    const unsigned int UNKNOWN_LENGTH_ERROR = 5;
    const unsigned int MOVE_TO_REFERENCE_ERROR = 7;
    
    const unsigned int ALLOC_INIT_REQUIRED = 10;
	const unsigned int CONST_ALLOCATION_ERROR = 11;	// constants must be initialized in their allocation
    const unsigned int NON_CONST_VALUE_ERROR = 12;
    const unsigned int REFERENCE_ALLOCATION_ERROR = 13; // references must also be initialized
    const unsigned int STATIC_MEMORY_INITIALIZATION_ERROR = 15;
    
    const unsigned int DUPLICATE_SYMBOL_ERROR = 30; // The symbol already exists in that scope; cannot be redefined
	const unsigned int DUPLICATE_DEFINITION_ERROR = 31;	// The definition for this resource was already found
	
    const unsigned int NON_MODIFIABLE_LVALUE_ERROR = 40;	// left hand expressions in assignments must be modifiable-lvalues
	const unsigned int REFERENCED_BEFORE_ASSIGNMENT_ERROR = 41;	// all symbols must be assigned before they can be referenced safely
    
    const unsigned int ILLEGAL_OPERATION_ERROR = 50;    // The statement is not allowed where it was found
    const unsigned int ILLEGAL_RETURN_ERROR = 51;   // Return statements must only occur within functions
	const unsigned int NO_RETURN_ERROR = 52;	// used for when not all control paths in a function return a value
    const unsigned int RETURN_AUTOMATIC_REFERENCE = 53; // references to automatic data is illegal

    const unsigned int INVALID_UNARY_OPERATOR_ERROR = 55;   // if the operator given for a unary is not a valid unary operator
    const unsigned int UNARY_TYPE_NOT_SUPPORTED = 56;   // if the unary operator is not supported with a given type
	const unsigned int UNDEFINED_OPERATOR_ERROR = 57;	// the operator used is undefined for the data type
    
    const unsigned int ILLEGAL_ADDRESS_OF_ARGUMENT = 61;    // the address-of operator may only be used with lvalues and member selection binary expressions
    const unsigned int ILLEGAL_INDIRECTION = 62;    // the * operator may only be used with pointer types
    const unsigned int ILLEGAL_THIS_PARAMETER = 65;

    const unsigned int SELF_CONTAINMENT_ERROR = 71; // a struct may not contain an instance of itself

    const unsigned int CONSTRUCTION_NUMBER_INIT_ERROR = 80; // constructions must initialize each member exactly once
    
    // Location / definition errors
    const unsigned int SYMBOL_NOT_FOUND_ERROR = 100;
    const unsigned int UNDEFINED_ERROR = 101;
    const unsigned int UNDEFINED_STRUCT_ACCESSION = 102;
    const unsigned int UNKNOWN_ATTRIBUTE = 103;
    const unsigned int DATA_FREED = 105;
    
    const unsigned int EMPTY_SCOPE_BLOCK = 111;
    
    const unsigned int SIGNATURE_ERROR = 120;
    
    const unsigned int OUT_OF_SCOPE_ERROR = 150;
    
    const unsigned int DECLARATION_ERROR = 160;
    const unsigned int INCLUDE_SCOPE_ERROR = 165;
    const unsigned int FILE_NOT_FOUND_ERROR = 166;
    
    const unsigned int CALLING_CONVENTION_ERROR = 170;
    
    const unsigned int INVISIBLE_SYMBOL = 180;  // see docs/Includes.md

    // Type errors
    const unsigned int INVALID_SYMBOL_TYPE_ERROR = 200;
    const unsigned int UNEXPECTED_FUNCTION_SYMBOL = 203;
    const unsigned int UNICODE_ERROR = 205;
    
    const unsigned int TYPE_ERROR = 210;
    const unsigned int VOID_TYPE_ERROR = 211;
    const unsigned int OPERATOR_TYPE_ERROR = 212;   // the specified operator could not be used on the given expression
    const unsigned int INVALID_CAST_ERROR = 213;
    const unsigned int NOT_SUBSCRIPTABLE_ERROR = 214;   // only strings and arrays may use []
    const unsigned int RETURN_MISMATCH_ERROR = 215; // a function's return type does not match its signature
    const unsigned int INCORRECT_THIS_TYPE = 219;

	const unsigned int TYPE_VALIDITY_RULE_VIOLATION_ERROR = 220;	// SIN has strict type validity rules, and one or more were violated
	const unsigned int STRUCT_TYPE_EXPECTED_ERROR = 225;	// to use the dot operator, the left-hand expression must be 'struct' type
    const unsigned int ILLEGAL_STRUCT_NAME = 226;

    const unsigned int QUALITY_CONFLICT_ERROR = 230;
    const unsigned int ILLEGAL_QUALITY_ERROR = 231;
	const unsigned int VARIABILITY_ERROR = 232;
	const unsigned int TYPE_MANAGEMENT_ERROR = 235;

    const unsigned int SIGNED_UNSIGNED_MISMATCH = 241;
    const unsigned int WIDTH_MISMATCH = 242;
    const unsigned int POTENTIAL_DATA_LOSS = 243;

    const unsigned int SIGNATURE_MISMATCH = 250;
    const unsigned int LIST_TYPE_MISMATCH = 253;

    const unsigned int DEDUCATION_FAILURE = 290;
    const unsigned int TEMPLATE_TYPE_DEDUCTION_FAILURE = 291;
    const unsigned int CONSTRUCTION_TYPE_DEDUCTION_FAILURE = 292;

    // Internal errors, etc.
    const unsigned int INVALID_EXPRESSION_TYPE_ERROR = 300;
    const unsigned int TUPLE_MEMBER_SELECTION_ERROR = 301;
    const unsigned int STRUCT_MEMBER_SELECTION_ERROR = 302;

    const unsigned int ILLEGAL_MOVE_ASSIGNMENT_EXPRESSION = 310;

    const unsigned int UNSUPPORTED_FEATURE = 390;

    // Parse errors
    const unsigned int INVALID_TOKEN = 400;
    const unsigned int BAD_LITERAL = 401;
    const unsigned int EXPECTED_SYMBOL_QUALITY = 402;
    const unsigned int MISSING_SEMICOLON_ERROR = 404;
    const unsigned int MISSING_GROUPING_SYMBOL_ERROR = 405;
    const unsigned int MISSING_IDENTIFIER_ERROR = 406;
    const unsigned int INVALID_TYPE_SYNTAX = 407;
    const unsigned int UNEXPECTED_KEYWORD_ERROR = 410;
    const unsigned int UNEXPECTED_SYMBOL_QUALITY = 412;

    const unsigned int EXPECTED_INITIALIZATION = 421;
    const unsigned int EXPECTED_LIST_INITIALIZATION = 422;
    
    const unsigned int INCOMPLETE_TYPE_ERROR = 430; // type information given was incomplete and could not be parsed

    // Warning codes
    const unsigned int UNSAFE_OPERATION = 501;
    const unsigned int BITSHIFT_RESULT = 502;

    const unsigned int MAIN_SIGNATURE = 510;    // 'main' should return an integer and have one argument

    // Note codes (not necessarily warnings, but alerts)
    const unsigned int NO_EFFECT = 900;
};
