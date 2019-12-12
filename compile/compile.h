/*

SIN Toolchain (x86 target)
compile.h

The main compiler class

Copyright 2019 Riley Lannon

*/

#include <string>
#include <unordered_map>

#include "symbol.h"
#include "../parser/Parser.h"

class compiler {
    std::string current_scope_name; // the name of the current scope
    unsigned int current_scope_level;   // the current scope level

    std::unordered_map<std::string, symbol> symbol_table;    // the symbol table will be implemented through an unordered map

    unsigned int max_offset;    // the maximum offset within the current stack frame -- use for new variables, calls, etc

    std::stringstream allocate(Allocation alloc_stmt);
    std::stringstream allocate_automatic(Allocation alloc_stmt);
public:
    compiler();
    ~compiler();
};
