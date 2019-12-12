/*

SIN Toolchain (x86 target)
CompilerErrorCodes.h

Some error code constants for the compiler error handler

Codes fall into the following categories:
    000 -   099:    Illegal operations
        Used for things like assigning to a const qualified variable
    100 -   199:    Location/undefined errors
        Used when something couldn't be found or resolved
    200 -   299:    Type error
        Used for things like type mismatches

*/

namespace compiler_errors {
    const unsigned int CONST_ASSIGNMENT_ERROR = 1;
    const unsigned int SYMBOL_NOT_FOUND_ERROR = 100;
};
