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
    const unsigned int CONST_ASSIGNMENT_ERROR = 1;
    const unsigned int DATA_WIDTH_ERROR = 3;
    const unsigned int DUPLICATE_SYMBOL_ERROR = 30; // The symbol already exists in that scope; cannot be redefined
    const unsigned int ILLEGAL_OPERATION_ERROR = 50;    // The statement is not allowed where it was found
    
    // Location / definition errors
    const unsigned int SYMBOL_NOT_FOUND_ERROR = 100;
    const unsigned int UNDEFINED_ERROR = 101;
    const unsigned int SIGNATURE_ERROR = 120;
    const unsigned int OUT_OF_SCOPE_ERROR = 150;

    // Type errors
    const unsigned int INVALID_SYMBOL_TYPE_ERROR = 200;
    const unsigned int UNEXPECTED_FUNCTION_SYMBOL = 203;
    const unsigned int UNICODE_ERROR = 205;
    const unsigned int TYPE_ERROR = 210;
    const unsigned int VOID_TYPE_ERROR = 211;
    const unsigned int OPERATOR_TYPE_ERROR = 212;   // the specified operator could not be used on the given expression
    const unsigned int QUALITY_CONFLICT_ERROR = 230;
    const unsigned int ILLEGAL_QUALITY_ERROR = 231;

    // Internal errors
    const unsigned int INVALID_EXPRESSION_TYPE_ERROR = 300;

    // Parse errors
    const unsigned int INVALID_TOKEN = 400;
    const unsigned int MISSING_SEMICOLON_ERROR = 404;
    const unsigned int MISSING_GROUPING_SYMBOL_ERROR = 405;
    const unsigned int MISSING_IDENTIFIER_ERROR = 406;
    const unsigned int UNEXPECTED_KEYWORD_ERROR = 410;
    const unsigned int INCOMPLETE_TYPE_ERROR = 430; // type information given was incomplete and could not be parsed
};
