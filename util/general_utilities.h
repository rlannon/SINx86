/*

SIN Toolchain (x86 target)
general_utilites.h
Copyright 2020 Riley Lannon

Contains some general compiler utilities

*/

#include "../parser/Statement.h"
#include "EnumeratedTypes.h"

namespace general_utilities {
    bool returns(StatementBlock to_check);
    bool returns(std::shared_ptr<Statement> to_check);
    bool ite_returns(IfThenElse *to_check);
    bool is_bitwise(exp_operator op);
}
