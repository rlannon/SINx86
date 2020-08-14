#pragma once

/*

SIN Toolchain (x86 target)
alloc_util.h
Copyright 2020 Riley Lannon

Contains utilities for allocation

*/

#include "constant_eval.h"
#include "struct_table.h"
#include "symbol_table.h"
#include "expression_util.h"

namespace alloc_util {
    size_t get_width(
        DataType &alloc_data,
        compile_time_evaluator evaluator,
        struct_table &structs,
        symbol_table &symbols,
        std::string scope_name, 
        unsigned int scope_level, 
        unsigned int line
    );
};
