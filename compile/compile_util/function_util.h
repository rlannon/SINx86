#pragma once

/*

SIN Compiler Toolchain (x86 target)
function_util.h
Copyright 2020 Riley Lannon

Contains a set of utilities for functions in SIN

*/

#include <string>
#include "../symbol.h"
#include "../function_symbol.h"

namespace function_util
{
    std::stringstream call_sre_free(symbol& s);
    std::stringstream call_sre_add_ref(symbol& s);
    std::stringstream call_sre_mam_util(symbol& s, std::string func_name);
    std::string call_sre_function(std::string func_name);

    template<typename T>
    function_symbol create_function_symbol(
        const T& def,
        bool mangle=true,
        bool defined=true,
        const std::string& scope_name = "global",
        unsigned int scope_level = 0, 
        bool is_method = false
    );

    std::string call_sincall_subroutine(std::string name);

    bool returns(StatementBlock to_check);
}
