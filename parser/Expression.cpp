// Expression.cpp
// Implementation of the Expression class

#include "Expression.h"


const bool is_literal(lexeme_type candidate_type) {
	if (candidate_type == INT_LEX || candidate_type == FLOAT_LEX || candidate_type == BOOL_LEX || candidate_type == STRING_LEX) {
		return true;
	}
	else {
		return false;
	}
}


bool Expression::is_const()
{
	return this->_const;
}

exp_type Expression::get_expression_type()
{
	return this->expression_type;
}

void Expression::set_const()
{
	// marks an expression as a constexpr
	this->_const = true;
}


Expression::Expression(exp_type expression_type) : expression_type(expression_type) {
	this->_const = false;	// all expressions default to being non-const
}

Expression::Expression(): Expression(EXPRESSION_GENERAL) {
	// call specialized constructor with default value
}

Expression::~Expression() {
}



DataType Literal::get_data_type() {
	return this->type;
}

std::string Literal::get_value() {
	return this->value;
}

Literal::Literal(Type data_type, std::string value, Type subtype) : value(value) {
	Literal::expression_type = LITERAL;

    // symbol qualities for our DataType object
    bool const_q = true;
    bool long_q = false;
    bool short_q = false;
    bool signed_q = false;
    
    // If we have an integer, parse the value to see if we can determine some qualities about it
    if (data_type == INT) {
        long val = std::stol(value);

        // signed/unsigned
        if (val < 0) {
            signed_q = true;
        }

        // long/short
        if (val >= 0x100000000) {
            long_q = true;
        } else if (val < 0x10000) {
            short_q = true;
        }

        // todo: handle long/short for signed numbers?
    }

    // set our symbol qualities
    symbol_qualities qualities(const_q, false, false, signed_q, !signed_q, long_q, short_q);  // literals are always considered const

    // todo: set long/short qualities for ints and floats
	Literal::type = DataType(data_type, subtype, qualities);
}

Literal::Literal() {
	Literal::expression_type = LITERAL;
	Literal::type = DataType();
}


std::string LValue::getValue() {
	return this->value;
}

std::string LValue::getLValueType() {
	return this->LValue_Type;
}

void LValue::setValue(std::string new_value) {
	this->value = new_value;
}

void LValue::setLValueType(std::string new_lvalue_type) {
	this->LValue_Type = new_lvalue_type;
}

LValue::LValue(std::string value, std::string LValue_Type) : value(value) {
	LValue::expression_type = LVALUE;
	LValue::LValue_Type = LValue_Type;
}

LValue::LValue(std::string value) : value(value) {
	LValue::expression_type = LVALUE;
	LValue::LValue_Type = "var";
}

LValue::LValue() {
	LValue::expression_type = LVALUE;
	LValue::value = "";
	LValue::LValue_Type = "var";
}


// Lists

std::vector<std::shared_ptr<Expression>> ListExpression::get_list()
{
	return this->list_members;
}

ListExpression::ListExpression(std::vector<std::shared_ptr<Expression>> list_members) : list_members(list_members)
{
	this->expression_type = LIST;
}

ListExpression::ListExpression() {
	this->expression_type = LIST;
	this->list_members = {};
}

ListExpression::~ListExpression() {

}


// Pointers

LValue AddressOf::get_target() {
	return this->target;
}

AddressOf::AddressOf(LValue target) : target(target) {
	AddressOf::expression_type = ADDRESS_OF;
}

AddressOf::AddressOf() {
	AddressOf::expression_type = ADDRESS_OF;
}


std::shared_ptr<Expression> Dereferenced::get_contained_expression() {
	return this->contained_expression;
}

Dereferenced::Dereferenced(std::shared_ptr<Expression> ptr) : contained_expression(ptr) {
	this->expression_type = DEREFERENCED;
}

Dereferenced::Dereferenced() {
	Dereferenced::expression_type = DEREFERENCED;
}



std::shared_ptr<Expression> Binary::get_left() {
	return this->left_exp;
}

std::shared_ptr<Expression> Binary::get_right() {
	return this->right_exp;
}

exp_operator Binary::get_operator() {
	return this->op;
}

Binary::Binary(std::shared_ptr<Expression> left_exp, std::shared_ptr<Expression> right_exp, exp_operator op) : left_exp(left_exp), right_exp(right_exp), op(op) {
	Binary::expression_type = BINARY;
}

Binary::Binary() {
	Binary::op = NO_OP;	// initialized to no_op so we don't ever run into uninitialized variables
	Binary::expression_type = BINARY;
}



exp_operator Unary::get_operator() {
	return this->op;
}

std::shared_ptr<Expression> Unary::get_operand() {
	return this->operand;
}

Unary::Unary(std::shared_ptr<Expression> operand, exp_operator op) : operand(operand), op(op) {
	Unary::expression_type = UNARY;
}

Unary::Unary() {
	Unary::op = NO_OP;	// initialize to no_op so we don't ever try to access uninitialized data
	Unary::expression_type = UNARY;
}



// Parsing function calls

std::shared_ptr<LValue> ValueReturningFunctionCall::get_name() {
	return this->name;
}

std::string ValueReturningFunctionCall::get_func_name() {
	return this->name->getValue();
}

std::vector<std::shared_ptr<Expression>> ValueReturningFunctionCall::get_args() {
	return this->args;
}

std::shared_ptr<Expression> ValueReturningFunctionCall::get_arg(int i) {
	return this->args[i];
}

int ValueReturningFunctionCall::get_args_size() {
	return this->args.size();
}

ValueReturningFunctionCall::ValueReturningFunctionCall(std::shared_ptr<LValue> name, std::vector<std::shared_ptr<Expression>> args) : name(name), args(args) {
	ValueReturningFunctionCall::expression_type = VALUE_RETURNING_CALL;
}

ValueReturningFunctionCall::ValueReturningFunctionCall() {
	ValueReturningFunctionCall::expression_type = VALUE_RETURNING_CALL;
}



// sizeof expressions

DataType SizeOf::get_type() {
	return this->to_check;
}

SizeOf::SizeOf(DataType to_check) : to_check(to_check) {
	this->expression_type = SIZE_OF;
}

SizeOf::SizeOf() {
	this->expression_type = SIZE_OF;
}

std::shared_ptr<Expression> Indexed::get_index_value()
{
	return this->index_value;
}

Indexed::Indexed(std::string value, std::string LValue_type, std::shared_ptr<Expression> index_init) : index_value(index_init)
{
	this->value = value;
	this->LValue_Type = LValue_type;
	this->expression_type = INDEXED;
}

Indexed::Indexed()
{
	this->expression_type = INDEXED;
}
