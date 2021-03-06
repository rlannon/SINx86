/*

SIN Toolchain (x86 target)
constant_eval.cpp
Copyright 2020 Riley Lannon

The implementation of our compile-time evaluation functionality

*/

#include "constant_eval.h"

std::string compile_time_evaluator::evaluate_literal(const Literal & exp)
{
	/*
	
	evaluate_literal
	Evaluates a literal expression

	*/

	return exp.get_value();
}

std::string compile_time_evaluator::evaluate_lvalue(const Identifier & exp, const std::string& scope_name, unsigned int scope_level, unsigned int line)
{
	/*
	
	evaluate_lvalue
	Evaluates a const lvalue expression (a const symbol)
	
	*/

	const_symbol to_return = this->lookup(exp.getValue(), scope_name, scope_level, line);
	return to_return.get_value();
}

std::string compile_time_evaluator::evaluate_unary(const Unary & exp, const std::string& scope_name, unsigned int scope_level, unsigned int line)
{
	/*
	
	evaluate_unary
	Evaluates a unary expression
	
	This must evaluate the expression based on the data type; it cannot just return a value contained elsewhere, like an lvalue or literal (unfortunately)

	*/

	// first, ensure we have an appropriate data type
	DataType exp_data_type = expression_util::get_expression_data_type(
        exp.get_operand(),
        *this->constants,
        *this->structs,
        line,
        nullptr
    );
	if (exp_data_type.get_primary() == BOOL || exp_data_type.get_primary() == INT || exp_data_type.get_primary() == FLOAT) {
		// evaluate the operand
		std::string evaluated = this->evaluate_expression(exp.get_operand(), scope_name, scope_level, line);

		// result will change based on operator type
		switch (exp.get_operator()) {
		case PLUS:
			// unary plus does nothing
			break;
		case MINUS:
			// unary minus only works on numeric types
			if (exp_data_type.get_primary() != BOOL) {
				// simply prefix a minus sign
				evaluated = "-(" + evaluated + ")";
			}
			else {
				throw UnaryTypeNotSupportedError(line);
			}
			break;
		case NOT:
			if (exp_data_type.get_primary() == BOOL) {
				if (evaluated == "true") {
					evaluated = "false";
				}
				else if (evaluated == "false") {
					evaluated = "true";
				}
				else {
					throw CompilerException("Invalid boolean value encountered when performing compile-time evaluation", compiler_errors::UNDEFINED_ERROR, line);
				}
			}
			else {
				throw UnaryTypeNotSupportedError(line);
			}
			break;
		case BIT_NOT:
			// todo: evaluate bit_not
			if (exp_data_type.get_primary() == INT) {

			}
			else if (exp_data_type.get_primary() == FLOAT) {

			}
			else {
				throw UnaryTypeNotSupportedError(line);
			}
			break;
		default:
			// todo: throw exception
			break;
		}

		return evaluated;
	}
	else {
		throw UnaryTypeNotSupportedError(line);
		return std::string();
	}
}

std::string compile_time_evaluator::evaluate_binary(const Binary & exp, const std::string& scope_name, unsigned int scope_level, unsigned int line)
{
	return std::string();
}

std::string compile_time_evaluator::evaluate_expression(const Expression &to_evaluate, const std::string& scope_name, unsigned int scope_level, unsigned int line)
{
	/*
	
	evaluate_expression
	Evaluates a constexpr

	This function will dispatch appropriately and return the final result

	@param	to_evaluate	The expression we wish to evaluate
	@param	scope_name	The name of the scope where the expression occurs (for variable selection)
	@param	scope_level	The scope block number (depth) where the expression occurs (again for variable selection)
	
	*/

	std::string evaluated_expression = "";

	if (to_evaluate.get_expression_type() == LITERAL) {
		auto &exp = static_cast<const Literal&>(to_evaluate);
		evaluated_expression = compile_time_evaluator::evaluate_literal(exp);
	}
	else if (to_evaluate.get_expression_type() == IDENTIFIER) {
		auto &lvalue = static_cast<const Identifier&>(to_evaluate);
		evaluated_expression = this->evaluate_lvalue(lvalue, scope_name, scope_level, line);
	}
	else if (to_evaluate.get_expression_type() == UNARY) {
		auto &unary = static_cast<const Unary&>(to_evaluate);
		evaluated_expression = this->evaluate_unary(unary, scope_name, scope_level, line);
	}
	else if (to_evaluate.get_expression_type() == LIST) {
		// for lists, just evaluate each element individually and concatenate
		auto &l = static_cast<const ListExpression&>(to_evaluate);
		for (auto elem: l.get_list()) {
			evaluated_expression += this->evaluate_expression(*elem, scope_name, scope_level, line) + ",";
		}
	}
	// todo: more expression types
	else {
		// throw an exception as the expression was invalid
		throw CompilerException("Could not evaluate compile-time constant; invalid expression type", compiler_errors::INVALID_EXPRESSION_TYPE_ERROR, line);
	}
	
	return evaluated_expression;
}

compile_time_evaluator::compile_time_evaluator(struct_table* structs)
{
	this->constants = new symbol_table();
	this->structs = structs;
}

compile_time_evaluator::compile_time_evaluator() {
	this->constants = new symbol_table();
	this->structs = nullptr;
}

compile_time_evaluator::~compile_time_evaluator()
{
    if (this->constants) {
        delete this->constants;
    }
}
