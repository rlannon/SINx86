/*

SIN toolchain (x86 target)
expression_util.h
Copyright 2020 Riley Lannon

Various utility functions for expression evaluation

*/

#pragma once

#include "utilities.h"

namespace expression_util {
    std::stringstream get_exp_address(
        const Expression &exp,
        symbol_table &symbols,
        struct_table &structs,
        reg r,
        unsigned int line
    );

    std::stringstream evaluate_member_selection(
        const Binary &to_evaluate,
        symbol_table &symbols,
        struct_table &structs,
        reg r,
        unsigned int line,
        bool dereference = true
    );

    DataType get_expression_data_type(
        const Expression &to_eval,
        symbol_table& symbols,
        struct_table& structs,
        unsigned int line,
        const DataType *type_hint = nullptr
    );

    size_t get_width(
        DataType &alloc_data,
        compile_time_evaluator &evaluator,
        struct_table &structs,
        symbol_table &symbols,
        std::string scope_name, 
        unsigned int scope_level, 
        unsigned int line
    );

    symbol &get_function_symbol(
        const Expression &func_name,
        struct_table &structs,
        symbol_table &symbols,
        unsigned int line
    );

    struct_info &get_struct_type(
        const Expression &exp,
        struct_table &structs,
        symbol_table &symbols,
        unsigned int line
    );
}
