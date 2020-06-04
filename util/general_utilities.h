/*

SIN Toolchain (x86 target)
general_utilites.h
Copyright 2020 Riley Lannon

Contains some general compiler utilities

*/

#include "../parser/Statement.h"

namespace general_utilities {
    bool returns(StatementBlock to_check);
    bool ite_returns(IfThenElse *to_check);
}
