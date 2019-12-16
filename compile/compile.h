/*

SIN Toolchain (x86 target)
compile.h

The main compiler class

Copyright 2019 Riley Lannon

*/

#include <string>
#include <sstream>
#include <unordered_map>

#include "symbol.h"
#include "../parser/Parser.h"
#include "compile_util/utilities.h"
#include "../util/stack.h"  // the stack data structure

class compiler {
    std::string current_scope_name; // the name of the current scope
    unsigned int current_scope_level;   // the current scope level

    stack<register_usage> reg_stack;    // a stack for tracking which registers are in use in a given scope

    std::unordered_map<std::string, std::shared_ptr<symbol>> symbol_table;    // the symbol table will be implemented through an unordered map

    unsigned int max_offset;    // the maximum offset within the current stack frame -- use for new variables, calls, etc

    // allocations
    std::stringstream allocate(Allocation alloc_stmt);
    symbol allocate_automatic(Allocation alloc_stmt);

    // assignments
    std::stringstream assign(Assignment assign_stmt);
    std::stringstream handle_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line);
    std::stringstream handle_int_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line);

    // utilities that require compiler's data members
    std::stringstream evaluate_expression(std::shared_ptr<Expression> to_evaluate, unsigned int line);
    std::stringstream evaluate_literal(Literal &to_evaluate, unsigned int line);
    std::stringstream evaluate_lvalue(LValue &to_evalue, unsigned int line);

    // We need to track the number for string constants, if/else blocks, etc.
    size_t strc_num;
    size_t if_else_num;

    // We have sections for the text, data, and bss segments here
    std::stringstream text_segment;
    std::stringstream data_segment;
    std::stringstream bss_segment;
public:
    compiler();
    ~compiler();
};
