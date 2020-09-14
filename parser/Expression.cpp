// Expression.cpp
// Implementation of the Expression class

#include "Expression.h"


const bool is_literal(lexeme_type candidate_type) {
	switch(candidate_type) {
		case INT_LEX:
		case FLOAT_LEX:
		case BOOL_LEX:
		case STRING_LEX:
		case CHAR_LEX:
			return true;
		default:
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

void Expression::override_qualities(symbol_qualities sq) {
	// base class override symbol qualities
	// todo: datatype for base?
}

bool Expression::has_type_information() const {
	return false;
}

bool Expression::was_overridden() const {
	return this->overridden;
}

Expression::Expression(exp_type expression_type) : expression_type(expression_type) {
	this->_const = false;	// all expressions default to being non-const
	this->overridden = false;
}

Expression::Expression(): Expression(EXPRESSION_GENERAL) {
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

void Literal::override_qualities(symbol_qualities sq) {
	// update the data type (for postfixed quality overrides)
	this->type.add_qualities(sq);	// todo: ensure the type override is valid
}

bool Literal::has_type_information() const {
	return true;
}

Literal::Literal(Type data_type, std::string value, Type subtype) : Expression(LITERAL), value(value) {
    // symbol qualities for our DataType object
    bool const_q = true;
    bool long_q = false;
    bool short_q = false;
    bool signed_q = true;

    // set our symbol qualities
    symbol_qualities qualities(const_q, false, false, signed_q, !signed_q, long_q, short_q);  // literals are always considered const
	this->type = DataType(data_type, subtype, qualities);
}

Literal::Literal(DataType t, std::string value): Expression(LITERAL) {
	this->type = t;
	this->value = value;
}

Literal::Literal(): Expression(LITERAL) {
	this->type = DataType();
}


std::string Identifier::getValue() {
	return this->value;
}

void Identifier::setValue(std::string new_value) {
	this->value = new_value;
}

Identifier::Identifier(std::string value) : Expression(IDENTIFIER), value(value) {
}

Identifier::Identifier(): Identifier("") {
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
	Expression(ATTRIBUTE),
	selected(selected)
{
	this->attrib = to_attribute(attribute_name);

	// set the type
	this->t = 
	DataType(
		INT,
		NONE,
		symbol_qualities(
			false,	// not const
			false,	// not static
			false,	// not dynamic
			false	// not signed
		)	// not long, short, or extern
	);

	// all attributes are final; they are not necessarily known at compile time, but they are not directly modifiable
	this->t.get_qualities().add_quality(FINAL);
}

AttributeSelection::AttributeSelection(std::shared_ptr<Binary> to_deconstruct): Expression(ATTRIBUTE)
{
	// Construct an 'AttributeSelection' object from a Binary expression
	
	// as long as we have a valid binary expression, continue
	if (to_deconstruct->get_right()->get_expression_type() == KEYWORD_EXP) {
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

bool ListExpression::has_type_information() const {
	return true;
}

Type ListExpression::get_list_type() const {
	return this->primary;
}

std::vector<std::shared_ptr<Expression>> ListExpression::get_list()
{
	return this->list_members;
}

ListExpression::ListExpression(std::vector<std::shared_ptr<Expression>> list_members, Type list_type) :
	Expression(LIST),
	list_members(list_members),
	primary(list_type)
{
}

ListExpression::ListExpression(): ListExpression({}, NONE) {
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
	Expression(KEYWORD_EXP),
	keyword(keyword)
{
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

Binary::Binary(
	std::shared_ptr<Expression> left_exp,
	std::shared_ptr<Expression> right_exp,
	exp_operator op
):
	Expression(BINARY),
	left_exp(left_exp),
	right_exp(right_exp),
	op(op)
{
}

Binary::Binary(): Expression(BINARY) {
	Binary::op = NO_OP;	// initialized to no_op so we don't ever run into uninitialized variables
}



exp_operator Unary::get_operator() {
	return this->op;
}

std::shared_ptr<Expression> Unary::get_operand() {
	return this->operand;
}

Unary::Unary(std::shared_ptr<Expression> operand, exp_operator op) : Expression(UNARY), operand(operand), op(op) {
}

Unary::Unary(): Expression(UNARY) {
	this->op = NO_OP;
}



// Parsing function calls

std::shared_ptr<Identifier> ValueReturningFunctionCall::get_name() {
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

ValueReturningFunctionCall::ValueReturningFunctionCall(
	std::shared_ptr<Identifier> name,
	std::vector<std::shared_ptr<Expression>> args
): 
	Expression(VALUE_RETURNING_CALL),
	name(name), 
	args(args) 
{
}

ValueReturningFunctionCall::ValueReturningFunctionCall(): Expression(VALUE_RETURNING_CALL) {
}


std::shared_ptr<Expression> Indexed::get_index_value()
{
	return this->index_value;
}

std::shared_ptr<Expression> Indexed::get_to_index()
{
	return this->to_index;
}

Indexed::Indexed(std::shared_ptr<Expression> to_index, std::shared_ptr<Expression> index_value): Expression(INDEXED)
{
	this->to_index = to_index;
	this->index_value = index_value;
}

Indexed::Indexed(): Indexed(nullptr, nullptr)
{
}

std::shared_ptr<Expression> Cast::get_exp() {
	return this->to_cast;
}

DataType& Cast::get_new_type() {
	return this->new_type;
}

Cast::Cast(std::shared_ptr<Expression> to_cast, DataType new_type): Expression(CAST) {
	this->to_cast = to_cast;
	this->new_type = new_type;
}

Cast::Cast(std::shared_ptr<Binary> b): Expression(CAST) {
	if (b->get_operator() == TYPECAST && b->get_right()->get_expression_type() == KEYWORD_EXP) {
		auto* kw = dynamic_cast<KeywordExpression*>(b->get_right().get());
		this->to_cast = b->get_left();
		this->new_type = kw->get_type();
	}
	else {
		this->expression_type = EXPRESSION_GENERAL;
		this->to_cast = nullptr;
	}
}
