// Expression.cpp
// Implementation of the Expression class

#include "Expression.h"


const bool is_literal(lexeme_type candidate_type) {
	if (
		candidate_type == INT_LEX || 
		candidate_type == FLOAT_LEX || 
		candidate_type == BOOL_LEX || 
		candidate_type == STRING_LEX ||
		candidate_type == CHAR_LEX
	) {
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



void Literal::set_type(DataType t) {
	this->type = t;
}

DataType Literal::get_data_type() {
	return this->type;
}

std::string Literal::get_value() {
	return this->value;
}

Literal::Literal(Type data_type, std::string value, Type subtype) : value(value) {
	this->expression_type = LITERAL;

    // symbol qualities for our DataType object
    bool const_q = true;
    bool long_q = false;
    bool short_q = false;
    bool signed_q = true;
    
    // If we have an integer, check the value to see if we have a long int
    if (data_type == INT) {
        long val = std::stol(value);

        if (val >= 0x100000000) {
            long_q = true;
        }

        // todo: handle long/short for signed numbers?
    }

    // set our symbol qualities
    symbol_qualities qualities(const_q, false, false, signed_q, !signed_q, long_q, short_q);  // literals are always considered const

    // todo: set long/short qualities for ints and floats
	this->type = DataType(data_type, subtype, qualities);
}

Literal::Literal(DataType t, std::string value) {
	this->type = t;
	this->value = value;
}

Literal::Literal() {
	this->expression_type = LITERAL;
	this->type = DataType();
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


// Attribute Selection

std::shared_ptr<Expression> AttributeSelection::get_selected() {
	return this->selected;
}

attribute AttributeSelection::get_attribute() {
	return this->attrib;
}

DataType &AttributeSelection::get_data_type() {
	return this->t;
}

AttributeSelection::AttributeSelection(std::shared_ptr<Expression> selected, std::string attribute_name):
	selected(selected)
{
	this->expression_type = ATTRIBUTE;
	this->attrib = to_attribute(attribute_name);

	// set the type
	this->t = 
	DataType(
		INT,
		NONE,
		symbol_qualities(
			false,
			false,
			false,
			false,
			true
		)
	);

	// all attributes are final; they are not necessarily known at compile time, but they are not directly modifiable
	this->t.get_qualities().add_quality(FINAL);
}

AttributeSelection::AttributeSelection(std::shared_ptr<Binary> to_deconstruct)
{
	// Construct an 'AttributeSelection' object from a Binary expression
	
	// as long as we have a valid binary expression, continue
	if (to_deconstruct->get_right()->get_expression_type() == KEYWORD_EXP) {	
		this->expression_type = ATTRIBUTE;
		this->selected = to_deconstruct->get_left();
		auto right = dynamic_cast<KeywordExpression*>(to_deconstruct->get_right().get());
		this->attrib = to_attribute(right->get_keyword());
	}
	else {
		this->expression_type = EXPRESSION_GENERAL;
		this->selected = nullptr;
	}

	// set the attribute data type -- always returns 'int &unsigned final'
	this->t = DataType(
		INT,
		NONE,
		symbol_qualities(
			false,
			false,
			false,
			false,
			true
		)
	);
	
	// all attributes are final; they are not necessarily known at compile time, but they are not directly modifiable
	this->t.get_qualities().add_quality(FINAL);
}

attribute AttributeSelection::to_attribute(std::string to_convert) {
    if (to_convert == "len") {
        return LENGTH;
    }
    else if (to_convert == "size") {
        return SIZE;
    }
    else if (to_convert == "var") {
        return VARIABILITY;
    }
    else {
        return NO_ATTRIBUTE;
    }
}

bool AttributeSelection::is_attribute(std::string a) {
	return to_attribute(a) != NO_ATTRIBUTE;
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

// Keyword Expressions -- necessary for some expressions

std::string KeywordExpression::get_keyword() {
	return this->keyword;
}

DataType &KeywordExpression::get_type() {
	return this->t;
}

KeywordExpression::KeywordExpression(std::string keyword):
	keyword(keyword)
{
	this->expression_type = KEYWORD_EXP;
}

KeywordExpression::KeywordExpression(DataType t):
	KeywordExpression("")
{
	this->t = t;
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


std::shared_ptr<Expression> Indexed::get_index_value()
{
	return this->index_value;
}

std::shared_ptr<Expression> Indexed::get_to_index()
{
	return this->to_index;
}

Indexed::Indexed(std::shared_ptr<Expression> to_index, std::shared_ptr<Expression> index_value)
{
	this->to_index = to_index;
	this->index_value = index_value;
	this->expression_type = INDEXED;
}

Indexed::Indexed()
{
	this->expression_type = INDEXED;
}

std::shared_ptr<Expression> Cast::get_exp() {
	return this->to_cast;
}

DataType& Cast::get_new_type() {
	return this->new_type;
}

Cast::Cast(std::shared_ptr<Expression> to_cast, DataType new_type) {
	this->expression_type = CAST;
	this->to_cast = to_cast;
	this->new_type = new_type;
}

Cast::Cast(std::shared_ptr<Binary> b) {
	if (b->get_operator() == TYPECAST && b->get_right()->get_expression_type() == KEYWORD_EXP) {
		auto* kw = dynamic_cast<KeywordExpression*>(b->get_right().get());
		this->expression_type = CAST;
		this->to_cast = b->get_left();
		this->new_type = kw->get_type();
	}
	else {
		this->expression_type = EXPRESSION_GENERAL;
		this->to_cast = nullptr;
	}
}
