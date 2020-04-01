#pragma once

/*

SIN Toolchain (x86 target)
constant_eval.h
Copyright 2020 Riley Lannon

This file contains the function/class declarations required for the compiler's compile-time expression evaluation.

*/

#include <string>
#include <unordered_map>

#include "const_symbol.h"
#include "../../util/Exceptions.h"
#include "../../parser/Statement.h"	// includes "Expression.h"

class compile_time_evaluator {
	std::unordered_map<std::string, const_symbol> constants;
	const_symbol lookup(std::string sym_name, std::string scope_name, unsigned int scope_level);

	std::string get_mangled_name(std::string sym_name, std::string scope_name, unsigned int scope_level);

	std::string evaluate_literal(Literal& literal);
public:
	void add_constant(Allocation &alloc, symbol &s);

	std::string evaluate_expression(std::shared_ptr<Expression> to_evaluate, std::string scope_name, unsigned int scope_level);

	compile_time_evaluator();
	~compile_time_evaluator();
};
