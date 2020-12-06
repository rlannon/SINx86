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
#include "../parser/Parser.h"
#include "compile_util/utilities.h"
#include "compile_util/symbol_table.h"
#include "compile_util/struct_table.h"
#include "../util/stack.h"  // the stack data structure

#include "compile_util/constant_eval.h"
#include "compile_util/expression_util.h"
#include "compile_util/assign_util.h"
#include "compile_util/magic_numbers.h"

class compiler {
    /*

    compiler
    The code generator class for the compiler project

    */

    // The class containing our compiler
	std::string filename;
	std::string file_path;

	// compiler flags
	const bool _micro_mode;
	const bool _strict;
	const bool _allow_unsafe;

    // todo: break code generation into multiple friend classes

	compile_time_evaluator evaluator;	// the compile-time constant evaluator

    std::set<std::string> compiled_headers; // which headers have already been handled
	std::set<std::string> externals;	// symbols which use 'extern'

    std::string current_scope_name; // the name of the current scope
    unsigned int current_scope_level;   // the current scope level
    bool is_in_scope(symbol &sym);  // check if the symbol is in scope

    stack<register_usage> reg_stack;    // a stack for tracking which registers are in use in a given scope

    symbol_table symbols;    // todo: dynamically allocate?
	symbol *lookup(std::string name, unsigned int line);   // look up a symbol's name
    symbol &add_symbol(symbol &to_add, unsigned int line);	// add a symbol
    symbol &add_symbol(std::shared_ptr<symbol> to_add, unsigned int line);

	struct_table structs;
	void add_struct(struct_info to_add, unsigned int line);	// add a struct to the table
    struct_info& get_struct_info(std::string struct_name, unsigned int line);   // gets the data about a given struct

	// We need to track the number for string constants, if/else blocks, etc.
	size_t strc_num;
	size_t strcmp_num;
	size_t fltc_num;
	size_t list_literal_num;
	size_t scope_block_num;
	size_t rtbounds_num;

	// We should have stringstreams for the text, rodata, data, and bss segments
	std::stringstream text_segment;
	std::stringstream rodata_segment;
	std::stringstream data_segment;
	std::stringstream bss_segment;

	// We must also keep track of the maximum offset within the current stack frame -- use for new variables, calls, etc.
    size_t max_offset;

	// compile an entire statement block
	std::stringstream compile_ast(StatementBlock &ast, function_symbol *signature = nullptr);

	// a function to compile a single statement
	std::stringstream compile_statement(Statement &s, function_symbol *signature);

	// allocations
	std::stringstream allocate(Allocation alloc_stmt);

	// assignments
	std::stringstream handle_assignment(Assignment &a);	// copy assignment
	std::stringstream handle_move(Movement &m);	// move assignment
	std::stringstream handle_alloc_init(
		symbol &sym,
		Expression &rvalue,
		unsigned int line
	);
	std::stringstream assign(
		DataType lhs_type,
		DataType &rhs_type,
		assign_utilities::destination_information dest,
		Expression &rvalue,
		unsigned int line,
		bool is_alloc_init = false
	);
	std::stringstream move(
		DataType &lvalue_type,
		DataType &rvalue_type,
		assign_utilities::destination_information dest,
		Expression &rvalue,
		unsigned int line
	);

	// todo: handle assignments for char, float, etc.

	// declarations
	std::stringstream handle_declaration(Declaration decl_stmt);

	// functions
	std::stringstream define_function(FunctionDefinition &definition);
    std::stringstream define_function(function_symbol &func_sym, StatementBlock prog, unsigned int line);

	std::pair<std::string, size_t> call_function(Procedure &to_call, unsigned int line, bool allow_void = true);
	
    std::stringstream sincall(function_symbol s, std::vector<Expression*> args, unsigned int line);
    std::stringstream sincall(function_symbol s, std::vector<std::shared_ptr<Expression>> args, unsigned int line);

	std::stringstream system_v_call(function_symbol s, std::vector<Expression*> args, unsigned int line);
	std::stringstream win64_call(function_symbol s, std::vector<Expression*> args, unsigned int line);

	// returns
	std::stringstream handle_return(ReturnStatement &ret, function_symbol &signature);
	std::stringstream sincall_return(ReturnStatement &ret, DataType return_type);

	// utilities that require compiler's data members
	std::stringstream get_exp_address(Expression &to_evaluate, reg r, unsigned int line);
	std::pair<std::string, size_t> evaluate_expression(
		Expression &to_evaluate,
		unsigned int line,
		DataType *type_hint = nullptr
	);
	std::stringstream evaluate_literal(Literal &to_evaluate, unsigned int line, DataType *type_hint = nullptr);
	std::stringstream evaluate_identifier(Identifier &to_evaluate, unsigned int line);
	std::stringstream evaluate_indexed(Indexed &to_evaluate, unsigned int line);
	std::stringstream evaluate_unary(Unary &to_evaluate, unsigned int line, DataType *type_hint = nullptr);
	std::pair<std::string, size_t> evaluate_binary(Binary &to_evaluate, unsigned int line, DataType *type_hint = nullptr);
	std::stringstream get_address_of(Unary &u, reg r, unsigned int line);

	// process an included file
	std::stringstream process_include(std::string include_filename, unsigned int line);
public:
    // the compiler's entry function
    void generate_asm(std::string filename);

    compiler();
    ~compiler();
};
