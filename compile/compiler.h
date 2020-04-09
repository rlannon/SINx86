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
#include "function_symbol.h"
#include "struct_info.h"
#include "compile_util/member_selection.h"
#include "../parser/Parser.h"
#include "compile_util/utilities.h"
#include "compile_util/symbol_table.h"
#include "../util/stack.h"  // the stack data structure

class compiler {
    // The class containing our compiler

    // todo: break code generation into multiple friend classes

    std::set<std::string> compiled_headers; // which headers have already been handled

    std::string current_scope_name; // the name of the current scope
    unsigned int current_scope_level;   // the current scope level
    bool is_in_scope(symbol &sym);  // check if the symbol is in scope

    stack<register_usage> reg_stack;    // a stack for tracking which registers are in use in a given scope

    symbol_table symbols;    // todo: dynamically allocate?
	std::shared_ptr<symbol> lookup(std::string name, unsigned int line);   // look up a symbol's name
    template<typename T> void add_symbol(T &to_add, unsigned int line);	// add a symbol

    std::unordered_map<std::string, struct_info> struct_table;	// todo: refactor this and add a 'struct_table' class for maintainability
	void add_struct(struct_info to_add, unsigned int line);	// add a struct to the table
    struct_info& get_struct_info(std::string struct_name, unsigned int line);   // gets the data about a given struct

	// We need to track the number for string constants, if/else blocks, etc.
	size_t strc_num;
	size_t scope_block_num;

	// We should have stringstreams for the text, rodata, data, and bss segments
	std::stringstream text_segment;
	std::stringstream rodata_segment;
	std::stringstream data_segment;
	std::stringstream bss_segment;

	// We must also keep track of the maximum offset within the current stack frame -- use for new variables, calls, etc.
    size_t max_offset;

	// compile an entire statement block
	std::stringstream compile_ast(StatementBlock &ast, std::shared_ptr<function_symbol> signature = nullptr);

	// a function to compile a single statement
	std::stringstream compile_statement(std::shared_ptr<Statement> s, std::shared_ptr<function_symbol> signature);

	// allocations
	std::stringstream allocate(Allocation alloc_stmt);

	// assignments
	std::stringstream assign(Assignment assign_stmt);
	std::stringstream handle_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line);
	std::stringstream handle_int_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line);
	std::stringstream handle_bool_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line);
	std::stringstream handle_string_assignment(symbol &sym, std::shared_ptr<Expression> value, unsigned int line);
	// todo: handle assignments for char, float, etc.

	// declarations
	void handle_declaration(Declaration decl_stmt);

	// functions
	std::stringstream define_function(FunctionDefinition definition);

	template<typename T> std::stringstream call_function(T to_call, unsigned int line, bool allow_void = true);
	std::stringstream sincall(function_symbol s, std::vector<std::shared_ptr<Expression>> args, unsigned int line);
	std::stringstream ccall(function_symbol s, std::vector<std::shared_ptr<Expression>> args, unsigned int line);
	std::stringstream handle_return(ReturnStatement ret, function_symbol signature);
	std::stringstream sincall_return(ReturnStatement &ret, DataType return_type);

	// utilities that require compiler's data members
	std::stringstream evaluate_expression(std::shared_ptr<Expression> to_evaluate, unsigned int line);
	std::stringstream evaluate_literal(Literal &to_evaluate, unsigned int line);
	std::stringstream evaluate_lvalue(LValue &to_evaluate, unsigned int line);
	std::stringstream evaluate_indexed(Indexed &to_evaluate, unsigned int line);
	std::stringstream evaluate_sizeof(SizeOf &to_evaluate, unsigned int line);
	std::stringstream evaluate_unary(Unary &to_evaluate, unsigned int line);
	std::stringstream evaluate_binary(Binary &to_evaluate, unsigned int line);
	std::stringstream evaluate_arrow(Binary &arrow_exp, unsigned int line);
	std::stringstream evaluate_dot(Binary &dot_exp, unsigned int line);
public:
    // the compiler's entry function
    void generate_asm(std::string filename, Parser &p);

    compiler();
    ~compiler();
};
