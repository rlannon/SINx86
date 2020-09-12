#pragma once

/*

SIN Toolchain (x86 target)
constant_eval.h
Copyright 2020 Riley Lannon

This file contains the function/class declarations required for the compiler's compile-time expression evaluation.

*/

#include <string>
#include <unordered_map>
#include <memory>

#include "const_symbol.h"
#include "symbol_table.h"
#include "struct_table.h"
#include "../../util/Exceptions.h"
#include "../../parser/Statement.h"	// includes "Expression.h""

// must be forward-declared to avoid a circular dependency
namespace expression_util {
	DataType get_expression_data_type(
		Expression &to_eval,
		symbol_table& symbols,
		struct_table& structs,
		unsigned int line
	);
}

class compile_time_evaluator {
	// data members
	symbol_table* constants;
	struct_table* structs;	// it must have access to the struct table in case we have access to const members
	
	/*
	
	The following functions are considered 'utility' functions and so their implementation is located in constant_eval_util.cpp:
		- lookup
		- remove_symbols_in_scope
		- add_constant

	*/

	const_symbol lookup(std::string sym_name, std::string scope_name, unsigned int scope_level, unsigned int line) const;

	void leave_scope(std::string name, unsigned int level);

	static std::string evaluate_literal(Literal& exp);
	std::string evaluate_lvalue(Identifier& exp, std::string scope_name, unsigned int scope_level, unsigned int line);
	std::string evaluate_unary(Unary & exp, std::string scope_name, unsigned int scope_level, unsigned int line);
	std::string evaluate_binary(Binary & exp, std::string scope_name, unsigned int scope_level, unsigned int line);
public:
	void add_constant(Allocation &alloc, symbol &s);	// located in utility file

	std::string evaluate_expression(Expression &to_evaluate, std::string scope_name, unsigned int scope_level, unsigned int line);

	compile_time_evaluator();
	compile_time_evaluator(struct_table* structs);
	~compile_time_evaluator();
};
