/*

SIN Toolchain (x86 target)
function_symbol.h

Contains the declaration of the function_symbol class

*/

#pragma once

#include "symbol.h"

class function_symbol: public symbol {
    /*

    The class for function symbols
    Inherits from 'symbol' because they share many of the same traits

    */

    // Function arguments -- formal parameters should be stored as symbols (they are considered local variables, so they will be pushed first)
    std::vector<symbol> formal_parameters;

    register_usage arg_regs;    // the registers used by this signature

    // calling convention -- defaults to SIN
    calling_convention call_con;
public:
    std::vector<symbol> &get_formal_parameters();
    calling_convention get_calling_convention();

    register_usage get_arg_regs();

    // constructors
    function_symbol(std::string function_name, DataType return_type, std::vector<symbol> formal_parameters, calling_convention call_con = SINCALL);
    function_symbol();
    ~function_symbol();
};
