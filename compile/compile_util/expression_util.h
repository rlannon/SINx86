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
        Expression &exp,
        symbol_table &symbols,
        struct_table &structs,
        reg r,
        unsigned int line
    );

    std::stringstream evaluate_member_selection(
        Binary &to_evaluate,
        symbol_table &symbols,
        struct_table &structs,
        reg r,
        unsigned int line,
        bool dereference = true
    );

    DataType get_expression_data_type(
        Expression &to_eval,
        symbol_table& symbols,
        struct_table& structs,
        unsigned int line,
        DataType *type_hint = nullptr
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
}
