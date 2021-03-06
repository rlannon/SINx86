/*

SIN Toolchain (x86 target)
function_symbol.h

Contains the declaration of the function_symbol class

*/

#pragma once

#include "symbol.h"
#include "compile_util/register_usage.h"
#include "../util/general_utilities.h"

class function_symbol: public symbol {
    /*

    The class for function symbols
    Inherits from 'symbol' because they share many of the same traits

    */

    // Function arguments -- formal parameters should be stored as symbols (they are considered local variables, so they will be pushed first)
    std::vector< std::shared_ptr< symbol > > formal_parameters;
    register_usage arg_regs;    // the registers used by this signature
    bool _method;

    // calling convention -- defaults to SIN
    calling_convention call_con;
public:
    bool matches(const function_symbol& right) const;
    bool is_method() const;
    bool requires_this() const;

    const std::vector< std::shared_ptr<symbol> > &get_formal_parameters() const;
    calling_convention get_calling_convention() const;

    const register_usage& get_arg_regs() const;

    // constructors
    function_symbol(
        std::string function_name, 
        DataType return_type, 
        std::vector<symbol> formal_parameters,
        std::string scope_name,
        unsigned int scope_level,
        calling_convention call_con = SINCALL,
        bool defined = true,
        unsigned int line_defined = 0
    );
    function_symbol();
    ~function_symbol();
};
