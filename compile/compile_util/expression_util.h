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
        std::shared_ptr<Expression> exp,
        symbol_table &symbols,
        struct_table &structs,
        reg r,
        unsigned int line
    );

    DataType get_expression_data_type(
        std::shared_ptr<Expression> to_eval,
        symbol_table& symbols,
        struct_table& structs,
        unsigned int line
    );
}
