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
	symbol *lookup(const std::string& name, unsigned int line);   // look up a symbol's name
    symbol &add_symbol(symbol &to_add, unsigned int line);	// add a symbol
    symbol &add_symbol(std::shared_ptr<symbol> to_add, unsigned int line);

	struct_table structs;
	void add_struct(const struct_info& to_add, unsigned int line);	// add a struct to the table
    struct_info& get_struct_info(const std::string& struct_name, unsigned int line);   // gets the data about a given struct

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
	std::stringstream compile_statement(const Statement &s, function_symbol *signature);

	// allocations
	std::stringstream allocate(const Allocation& alloc_stmt);

	// assignments
	std::stringstream handle_assignment(const Assignment &a);	// copy assignment
	std::stringstream handle_move(const Movement &m);	// move assignment
	std::stringstream handle_alloc_init(
		const symbol &sym,
		const Expression &rvalue,
		unsigned int line
	);
	std::stringstream assign(
		const DataType& lhs_type,
		const DataType &rhs_type,
		const assign_utilities::destination_information& dest,
		const Expression &rvalue,
		unsigned int line,
		bool is_alloc_init = false
	);
	std::stringstream move(
		const DataType &lvalue_type,
		const DataType &rvalue_type,
		const assign_utilities::destination_information& dest,
		const Expression &rvalue,
		unsigned int line
	);

	// todo: handle assignments for char, float, etc.

	// construction
	std::string construct_object(const ConstructionStatement& s);

	// declarations
	std::stringstream handle_declaration(const Declaration& decl_stmt);

	// functions
	std::stringstream define_function(const FunctionDefinition &definition);
    std::stringstream define_function(function_symbol &func_sym, StatementBlock prog, unsigned int line);

	std::pair<std::string, size_t> call_function(const Procedure &to_call, unsigned int line, bool allow_void = true);
	
    std::stringstream sincall(const function_symbol& s, std::vector<const Expression*> args, unsigned int line);
    std::stringstream sincall(const function_symbol& s, std::vector<std::unique_ptr<Expression>>& args, unsigned int line);

	std::stringstream system_v_call(const function_symbol& s, std::vector<Expression*> args, unsigned int line);
	std::stringstream win64_call(const function_symbol& s, std::vector<Expression*> args, unsigned int line);

	// returns
	std::stringstream handle_return(const ReturnStatement &ret, function_symbol &signature);
	std::stringstream sincall_return(const ReturnStatement &ret, DataType return_type);

	// utilities that require compiler's data members
	std::stringstream get_exp_address(const Expression &to_evaluate, reg r, unsigned int line);
	std::pair<std::string, size_t> evaluate_expression(
		const Expression &to_evaluate,
		unsigned int line,
		const DataType *type_hint = nullptr
	);
	std::stringstream evaluate_literal(const Literal &to_evaluate, unsigned int line, const DataType *type_hint = nullptr);
	std::stringstream evaluate_identifier(const Identifier &to_evaluate, unsigned int line);
	std::stringstream evaluate_indexed(const Indexed &to_evaluate, unsigned int line);
	std::stringstream evaluate_unary(const Unary &to_evaluate, unsigned int line, const DataType *type_hint = nullptr);
	std::pair<std::string, size_t> evaluate_binary(const Binary &to_evaluate, unsigned int line, const DataType *type_hint = nullptr);
	std::stringstream get_address_of(const Unary &u, reg r, unsigned int line);

	// process an included file
	std::stringstream process_include(std::string include_filename, unsigned int line);

	// issue a warning or throw an exception based on flags
	void _warn(const std::string& message, const unsigned int code, const unsigned int line);
public:
    // the compiler's entry function
    void generate_asm(const std::string& infile_name, std::string outfile_name);

    compiler(bool allow_unsafe, bool strict, bool use_micro);
    ~compiler();
};
