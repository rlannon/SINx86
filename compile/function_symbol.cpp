/*

SIN Toolchain (x86)
function_symbol.cpp

Implementation of the function_symbol class

*/

#include "symbol.h"

// for the time being, at least, all functions must be in the global scope at level 0
function_symbol::function_symbol(std::string function_name, DataType return_type, std::vector<symbol> formal_parameters) :
    symbol(function_name, "global", 0, return_type, 0), formal_parameters(formal_parameters)
{
    // todo: misc
}

function_symbol::function_symbol(): symbol() {
    // todo: constructor    
}

function_symbol::~function_symbol() {
    // todo: destructor
}
