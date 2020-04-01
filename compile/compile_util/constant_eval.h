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
	static std::string get_mangled_name(std::string sym_name, std::string scope_name, unsigned int scope_level);
	std::unordered_map<std::string, const_symbol> constants;
	const_symbol lookup(std::string sym_name, std::string scope_name, unsigned int scope_level, unsigned int line) const;

	void remove_symbols_in_scope(std::string scope_name, unsigned int scope_level);

	static std::string evaluate_literal(Literal& exp);
	std::string evaluate_lvalue(LValue& exp, std::string scope_name, unsigned int scope_level, unsigned int line);
public:
	void add_constant(Allocation &alloc, symbol &s);

	std::string evaluate_expression(std::shared_ptr<Expression> to_evaluate, std::string scope_name, unsigned int scope_level, unsigned int line);

	compile_time_evaluator();
	~compile_time_evaluator();
};
