/*

SIN Toolchain (x86)
function_symbol.cpp

Implementation of the function_symbol class

*/

#include "symbol.h"

calling_convention function_symbol::get_calling_convention() {
    // Get the function's calling convention
    return this->call_con;
}

// for the time being, at least, all functions must be in the global scope at level 0
function_symbol::function_symbol(std::string function_name, DataType return_type, std::vector<symbol> formal_parameters, calling_convention call_con) :
    symbol(function_name, "global", 0, return_type, 0), formal_parameters(formal_parameters),
    call_con(call_con)  // the calling convention should default to SINCALL
{
    this->symbol_type = FUNCTION_SYMBOL;
}

function_symbol::function_symbol() :
    function_symbol("", NONE, {})
{
    // delegate to specialized constructor
}

function_symbol::~function_symbol() {
    // todo: destructor
}
