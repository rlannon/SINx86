/*

SIN Toolchain (x86 target)
assign_util.h
Copyright 2020 Riley Lannon

Utilities for assignment -- specifically, tools to determine the source and destination operand locations

*/

#pragma once

#include <sstream>
#include <string>
#include <memory>

#include "utilities.h"
#include "expression_util.h"

namespace assign_utilities {
    std::pair<std::string, std::string> fetch_destination_operand(
        std::shared_ptr<Expression> exp,
        symbol_table &symbols,
        struct_table &structures,
        std::string scope_name,
        unsigned int scope_level,
        unsigned int line,
        reg r = RBX,
        bool is_initialization = false
    );
    std::pair<std::string, std::string> fetch_destination_operand(
        symbol &sym,
        symbol_table &symbols,
        unsigned int line,
        reg r = RBX,
        bool is_initialization = false
    );
    bool requires_copy(DataType &t);
};
