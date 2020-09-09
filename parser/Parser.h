/*

SIN Toolchain
Parser.h
Copyright 2019 Riley Lannon

The definition of the Parser class, which is responsible for taking a list of tokens generated by the Lexer and generating a parse tree.

Note that:
	- Parser.cpp contains the implementation of some of our general/utility functions
	- ParseStatement.cpp contains the implementation of the statement parsing functions
	- ParseExpression.cpp contains the implementation of the expression parsing functions, such as parse_expression and maybe_binary

*/

#pragma once

#include <iostream>
#include <fstream>
#include <memory>
#include <vector>
#include <string>
#include <tuple>
#include <unordered_map>

#include "type_deduction.h"
#include "Statement.h"
#include "Expression.h"
#include "Lexer.h"

#include "../util/Exceptions.h"	// ParserException
#include "../util/DataType.h"	// type information
#include "../util/general_utilities.h"
#include "../util/EnumeratedTypes.h"

class Parser
{
	// token trackers
	std::vector<lexeme> tokens;
	size_t position;
	size_t num_tokens;

	// the name of the file being parsed
	std::string filename;

	// Sentinel variable
	bool quit;

	// translates an operator character into an exp_operator type
	static const exp_operator translate_operator(std::string op_string);	// given the string name for an exp_operator, returns that exp_operator
	static const exp_operator make_compound_operator(lexeme l, lexeme r);
	static const bool is_valid_copy_assignment_operator(exp_operator op);
	static const bool is_valid_move_assignment_operator(exp_operator op);
	exp_operator read_operator(bool peek);

	// our operator and precedence handlers
	static const std::unordered_map<std::string, exp_operator> op_strings;
	static const std::unordered_map<exp_operator, size_t> op_precedence;	// todo: create operator class
	static const size_t get_precedence(std::string symbol, size_t line = 0);
	static const size_t get_precedence(exp_operator op, size_t line = 0);

	// Some utility functions
	bool is_at_end();	// tells us whether we have run out of tokens
	lexeme peek();	// get next token without moving the position
	lexeme next();	// get next token
	lexeme current_token();	// get token at current position
	lexeme previous();	// similar to peek; get previous token without moving back
	lexeme back();	// move backward one
	void skipPunc(char punc);	// skips the specified punctuation mark
	static bool is_type(std::string lex_value);
	static std::string get_closing_grouping_symbol(std::string beginning_symbol);
	static bool is_opening_grouping_symbol(std::string to_test);
	static const bool has_return(StatementBlock to_test);
	static const exp_operator get_unary_operator(std::string s);	// located in ParserUtil.cpp
	static const bool is_valid_operator(lexeme l);

	// get the appropriate SymbolQuality member from the lexeme containing it
	static SymbolQuality get_quality(lexeme quality_token);

	// we have to fetch a type (and its qualities) more than once; use a tuple for this
	DataType get_type(std::string grouping_symbol = "");
	DataType parse_subtype(std::string grouping_symbol = "");
	symbol_qualities get_prefix_qualities(std::string grouping_symbol = "");
	symbol_qualities get_postfix_qualities(std::string grouping_symbol="");		// symbol qualities can be placed after an allocation using the & operator

	// A utility function to give a calling convention from symbol qualities
	static calling_convention get_calling_convention(symbol_qualities sq, unsigned int line);

	// Parsing statements -- each statement type will use its own function to return a statement of that type
	std::shared_ptr<Statement> parse_statement(bool is_function_parameter = false);		// entry function to parse a statement

	std::shared_ptr<Statement> parse_include(lexeme current_lex);
	std::shared_ptr<Statement> parse_declaration(lexeme current_lex, bool is_function_parameter = false);
	std::shared_ptr<Statement> parse_ite(lexeme current_lex);
	std::shared_ptr<Statement> parse_allocation(lexeme current_lex, bool is_function_parameter = false);
	std::shared_ptr<Statement> parse_assignment(lexeme current_lex);
	std::shared_ptr<Statement> parse_move(lexeme current_lex);
	std::shared_ptr<Statement> parse_return(lexeme current_lex);
	std::shared_ptr<Statement> parse_while(lexeme current_lex);

	// We have a few different types of definitions we could parse; delegate
	std::shared_ptr<Statement> parse_definition(lexeme current_lex);
	std::shared_ptr<Statement> parse_function_definition(lexeme current_lex);
	std::shared_ptr<Statement> parse_struct_definition(lexeme current_lex);

	std::shared_ptr<Statement> parse_function_call(lexeme current_lex);

	// Parsing expressions

	/*
	put default argument here because we call "parse_expression" in "maybe_binary"; as a reuslt, "his_prec" appears as if it is being passed to the next maybe_binary, but isn't because we parse an expression before we parse the binary, meaning my_prec gets set to 0, and not to his_prec as it should
	Note we also have a 'not_binary' flag here; if the expression is indexed, we may not want to have a binary expression parsed
	*/
	std::shared_ptr<Expression> parse_expression(
		size_t prec=0,
		std::string grouping_symbol = "(",
		bool not_binary = false,
		bool omit_equals = false
	);
	std::shared_ptr<Expression> maybe_binary(
		std::shared_ptr<Expression> left,
		size_t my_prec,
		std::string grouping_symbol = "(",
		bool omit_equals = false
	);	// check to see if we need to fashion a binary expression
	static std::shared_ptr<Binary> create_compound_assignment_rvalue(
		std::shared_ptr<Expression> left,
		std::shared_ptr<Expression> right,
		exp_operator op
	);
public:
	// our entry function
	StatementBlock create_ast();

	Parser(std::string filename);
	~Parser();
};
