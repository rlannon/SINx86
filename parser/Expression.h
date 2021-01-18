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


bool is_literal(const lexeme_type candidate_type);

// Base class for all expressions
class Expression
{
protected:
	bool _const;	// if we have the 'constexpr' keyword, this will be set
	bool overridden;
	exp_type expression_type;	// replace "string type" with "exp_type expression_type"
public:
    bool is_const() const;
	void set_const();
	exp_type get_expression_type() const;

	virtual void override_qualities(symbol_qualities sq);
	virtual bool has_type_information() const;
	bool was_overridden() const;

	Expression(const exp_type expression_type);
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
	Type primary;
	std::vector<std::unique_ptr<Expression>> list_members;
public:
	std::vector<const Expression*> get_list() const;
	bool has_type_information() const override;
	Type get_list_type() const;	// the list type that we parsed -- () yields TUPLE, {} yields ARRAY

    void add_item(std::unique_ptr<Expression> to_add, const size_t index);

	ListExpression(std::vector<std::unique_ptr<Expression>>& list_members, Type list_type);
	ListExpression(std::unique_ptr<Expression> arg, Type list_type);
	ListExpression();
};

class Indexed : public Expression
{
	std::unique_ptr<Expression> index_value;	// the index value is simply an expression
	std::unique_ptr<Expression> to_index;	// what we are indexing
public:
	Expression &get_index_value();
	Expression &get_to_index();

	Indexed(std::unique_ptr<Expression> to_index, std::unique_ptr<Expression> index_value);
	Indexed();
};

class KeywordExpression: public Expression
{
	DataType t;
	std::string keyword;
public:
    bool has_type_information() const override;
    void override_qualities(symbol_qualities sq) override;

	const std::string& get_keyword() const;
	const DataType &get_type() const;
	KeywordExpression(std::string keyword);
	KeywordExpression(DataType t);
};

// Address Of -- the address of a variable
class AddressOf : public Expression
{
	std::shared_ptr<Expression> target;
public:
	Expression &get_target();

    AddressOf(std::shared_ptr<Expression> target);
	AddressOf();
};

class AttributeSelection;
class Cast;
class Binary : public Expression
{
	friend class AttributeSelection;
	friend class Cast;

	exp_operator op;	// +, -, etc.
	std::unique_ptr<Expression> left_exp;
	std::unique_ptr<Expression> right_exp;

	std::unique_ptr<Expression> get_left_unique();
	std::unique_ptr<Expression> get_right_unique();
public:
	const Expression &get_left() const;
	const Expression &get_right() const;

	exp_operator get_operator() const;

	Binary(std::unique_ptr<Expression> left, std::unique_ptr<Expression> right, const exp_operator op);
	Binary();
};

class Unary : public Expression
{
	exp_operator op;
	std::unique_ptr<Expression> operand;
public:
	exp_operator get_operator() const;
	const Expression &get_operand() const;

	Unary(std::unique_ptr<Expression> operand, const exp_operator op);
	Unary();
};


// Functions are expressions if they return a value
class Procedure: public Expression
{
    std::unique_ptr<Expression> name;
    std::unique_ptr<ListExpression> args;
public:
    const Expression &get_func_name() const;
    const ListExpression &get_args() const;
    const Expression &get_arg(size_t arg_no) const;
    size_t get_num_args() const;

    void insert_arg(std::unique_ptr<Expression> to_insert, const size_t index);

	Procedure(Procedure& other);
    Procedure(std::unique_ptr<Expression> proc_name, std::unique_ptr<ListExpression> proc_args);
    Procedure();
};

class CallExpression : public Procedure
{
public:
	CallExpression(Procedure *proc);
	CallExpression(CallExpression& other);
	CallExpression();
};

// typecasting expressions
class Cast : public Expression
{
	std::unique_ptr<Expression> to_cast;	// any expression can be casted
	DataType new_type;	// the new type for the expression
public:
    Expression &get_exp();
	DataType &get_new_type();

    Cast(Cast &old);
    Cast(std::unique_ptr<Expression> to_cast, const DataType& new_type);
	Cast(Binary &b);
};

// Attribute selection
class AttributeSelection : public Expression
{
	std::unique_ptr<Expression> selected;
	attribute attrib;
	DataType t;
public:
	static attribute to_attribute(std::string to_convert);
	static bool is_attribute(std::string a);

    Expression &get_selected();
	attribute get_attribute();
	DataType &get_data_type();

    AttributeSelection(AttributeSelection &old);
	AttributeSelection(std::unique_ptr<Expression> selected, const std::string& attribute_name);
	AttributeSelection(std::unique_ptr<Binary> to_deconstruct);
};
