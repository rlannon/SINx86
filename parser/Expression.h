/*

SIN Toolchain
Copyright 2019 Riley Lannon
Expression.h

Contains the Expression class and all of its child classes; these are used by the Parser and Compiler to contain all of the different types of expressions available in the language.

*/

#pragma once
#include <memory>
#include <vector>
#include <string>
#include <tuple>
#include <exception>

#include "../util/EnumeratedTypes.h"
#include "../util/DataType.h"


const bool is_literal(lexeme_type candidate_type);

// Base class for all expressions
class Expression
{
protected:
	bool _const;	// if we have the 'constexpr' keyword, this will be set
	exp_type expression_type;	// replace "string type" with "exp_type expression_type"
public:
	bool is_const();
	void set_const();
	exp_type get_expression_type();

	virtual void override_qualities(symbol_qualities sq);
	virtual bool has_type_information() const;

	Expression(exp_type expression_type);
	Expression();

	virtual ~Expression();
};

// Derived classes

class Literal : public Expression
{
	DataType type;
	std::string value;
public:
	void set_type(DataType t);
	DataType get_data_type();
	std::string get_value();

	void override_qualities(symbol_qualities sq) override;
	bool has_type_information() const override;

	Literal(Type data_type, std::string value, Type subtype = NONE);
	Literal(DataType t, std::string value);
	Literal();
};

// Identifier -- a variable name, function name, etc.
class Identifier : public Expression
{
protected:
	std::string value;	// the name of the variable
public:
	std::string getValue();
	void setValue(std::string new_value);
	
	Identifier(std::string value);
	Identifier();
};

class ListExpression : public Expression
{
	std::vector<std::shared_ptr<Expression>> list_members;
public:
	std::vector<std::shared_ptr<Expression>> get_list();

	ListExpression(std::vector<std::shared_ptr<Expression>> list_members);
	ListExpression();
	~ListExpression();
};

class Indexed : public Expression
{
	std::shared_ptr<Expression> index_value;	// the index value is simply an expression
	std::shared_ptr<Expression> to_index;	// what we are indexing
public:
	std::shared_ptr<Expression> get_index_value();
	std::shared_ptr<Expression> get_to_index();

	Indexed(std::shared_ptr<Expression> to_index, std::shared_ptr<Expression> index_value);
	Indexed();
};

class KeywordExpression: public Expression
{
	DataType t;
	std::string keyword;
public:
	std::string get_keyword();
	DataType &get_type();
	KeywordExpression(std::string keyword);
	KeywordExpression(DataType t);
};

// Address Of -- the address of a variable
class AddressOf : public Expression
{
	std::shared_ptr<Expression> target;
public:
	std::shared_ptr<Expression> get_target();

	AddressOf(std::shared_ptr<Expression> target);
	AddressOf();
};

class Binary : public Expression
{
	exp_operator op;	// +, -, etc.
	std::shared_ptr<Expression> left_exp;
	std::shared_ptr<Expression> right_exp;
public:
	std::shared_ptr<Expression> get_left();
	std::shared_ptr<Expression> get_right();

	exp_operator get_operator();

	Binary(std::shared_ptr<Expression> left, std::shared_ptr<Expression> right, exp_operator op);
	Binary();
};

class Unary : public Expression
{
	exp_operator op;
	std::shared_ptr<Expression> operand;
public:
	exp_operator get_operator();
	std::shared_ptr<Expression> get_operand();

	Unary(std::shared_ptr<Expression> operand, exp_operator op);
	Unary();
};


// Functions are expressions if they return a value

class ValueReturningFunctionCall : public Expression
{
	std::shared_ptr<Identifier> name;
	std::vector<std::shared_ptr<Expression>> args;
public:
	std::shared_ptr<Identifier> get_name();
	std::string get_func_name();
	std::vector<std::shared_ptr<Expression>> get_args();
	std::shared_ptr<Expression> get_arg(int i);
	int get_args_size();

	ValueReturningFunctionCall(std::shared_ptr<Identifier> name, std::vector<std::shared_ptr<Expression>> args);
	ValueReturningFunctionCall();
};

// typecasting expressions
class Cast : public Expression
{
	std::shared_ptr<Expression> to_cast;	// any expression can by typecast
	DataType new_type;	// the new type for the expression
public:
	std::shared_ptr<Expression> get_exp();
	DataType &get_new_type();
	Cast(std::shared_ptr<Expression> to_cast, DataType new_type);
	Cast(std::shared_ptr<Binary> b);
};

// Attribute selection
class AttributeSelection : public Expression
{
	std::shared_ptr<Expression> selected;
	attribute attrib;
	DataType t;
public:
	static attribute to_attribute(std::string to_convert);
	static bool is_attribute(std::string a);

	std::shared_ptr<Expression> get_selected();
	attribute get_attribute();
	DataType &get_data_type();
	AttributeSelection(std::shared_ptr<Expression> selected, std::string attribute_name);
	AttributeSelection(std::shared_ptr<Binary> to_deconstruct);
};
