/*

SIN Toolchain (x86 target)
compile.h

The main compiler class

Copyright 2019 Riley Lannon

*/

#pragma once

#include <string>
#include <sstream>
#include <unordered_map>
#include <set>

#include "symbol.h"
#include "struct_info.h"
#include "../parser/Parser.h"
#include "compile_util/utilities.h"
#include "../util/stack.h"  // the stack data structure

class compiler {
    // The class containing our compiler

    // todo: break code generation into multiple friend classes

    std::set<std::string> compiled_headers; // which headers have already been handled

    std::string current_scope_name; // the name of the current scope
    unsigned int current_scope_level;   // the current scope level
    bool is_in_scope(symbol &sym);  // check if the symbol is in scope

    stack<register_usage> reg_stack;    // a stack for tracking which registers are in use in a given scope

    std::unordered_map<std::string, std::shared_ptr<symbol>> symbol_table;    // the symbol table will be implemented through an unordered map
    std::shared_ptr<symbol> lookup(std::string name, unsigned int line);   // look up a symbol's name
    void add_symbol(symbol &to_add, unsigned int line);  // add a symbol, handle exceptions accordingly

    std::unordered_map<std::string, struct_info> struct_table;
    struct_info get_struct_info(std::string struct_name, unsigned int line);   // gets the data about a given struct

    size_t max_offset;    // the maximum offset within the current stack frame -- use for new variables, calls, etc

    // compile an entire statement block
    std::stringstream compile_ast(StatementBlock ast);
    
    // a function to compile a single statement
    std::stringstream compile_statement(std::shared_ptr<Statement> s);

    // allocations
    std::stringstream allocate(Allocation alloc_stmt);

    // assignments
    std::stringstream assign(Assignment assign_stmt);
    std::stringstream handle_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line);
    std::stringstream handle_int_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line);
    std::stringstream handle_bool_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line);
    std::stringstream handle_string_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line);
    // todo: handle assignments for char, float, etc.

    // functions
    std::stringstream define_function(FunctionDefinition definition);
    std::stringstream call_function(Call call);
    std::stringstream sincall(function_symbol s, std::vector<std::shared_ptr<Expression>> args, unsigned int line);

    // utilities that require compiler's data members
    std::stringstream evaluate_expression(std::shared_ptr<Expression> to_evaluate, unsigned int line);
    std::stringstream evaluate_literal(Literal &to_evaluate, unsigned int line);
    std::stringstream evaluate_lvalue(LValue &to_evaluate, unsigned int line);
	std::stringstream evaluate_indexed(Indexed &to_evaluate, unsigned int line);
    std::stringstream evaluate_sizeof(SizeOf &to_evaluate, unsigned int line);

    // We need to track the number for string constants, if/else blocks, etc.
    size_t strc_num;
    size_t scope_block_num;

    // We have sections for the text, data, and bss segments here
    std::stringstream text_segment;
    std::stringstream data_segment;
    std::stringstream bss_segment;
public:
    // the compiler's entry function
    void generate_asm(std::string filename, Parser &p);

    compiler();
    ~compiler();
};
