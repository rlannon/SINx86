/*

SIN Toolchain (x86 target)
general_utilites.h
Copyright 2020 Riley Lannon

Contains some general compiler utilities

*/

#pragma once

#include "../parser/Statement.h"
#include "EnumeratedTypes.h"

namespace general_utilities {
    const int BASE_PARAMETER_OFFSET = 16;

    bool returns(const StatementBlock& to_check);
    bool returns(const Statement &to_check);
    bool ite_returns(const IfThenElse *to_check);
    bool is_bitwise(const exp_operator op);
}
