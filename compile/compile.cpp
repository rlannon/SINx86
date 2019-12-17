/*

SIN Toolchain (x86 target)
compile.cpp

Implementation of the compiler class

Copyright 2019 Riley Lannon

*/

#include "compile.h"

bool compiler::is_in_scope(symbol &sym) {
    /*

    Determines whether the symbol in question is available in the current scope

    A valid symbol will be:
        - In the "global" scope OR in the current scope
        - Have a scope level less than or equal to the current level
    or:
        - Be a static variable

    */

    return ( sym.get_data_type().get_qualities().is_static() ||
        (( sym.get_scope_name() == "global" || 
            sym.get_scope_name() == this->current_scope_name
        ) && sym.get_scope_level() <= this->current_scope_level)
    );
}

compiler::compiler() {
    // todo: constructor
}

compiler::~compiler() {
    // todo: destructor
}
