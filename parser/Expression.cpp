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

std::unique_ptr<Expression> Expression::get_unique() {
    return std::make_unique<Expression>(*this);
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

std::unique_ptr<Expression> Literal::get_unique() {
    return std::make_unique<Literal>(*this);
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

std::unique_ptr<Expression> Identifier::get_unique() {
    return std::make_unique<Identifier>(*this);
}

Identifier::Identifier(std::string value) : Expression(IDENTIFIER), value(value) {
}

Identifier::Identifier(): Identifier("") {
}


// Attribute Selection

Expression &AttributeSelection::get_selected() {
	return *this->selected.get();
}

attribute AttributeSelection::get_attribute() {
	return this->attrib;
}

DataType &AttributeSelection::get_data_type() {
	return this->t;
}

AttributeSelection::AttributeSelection(Expression &selected, std::string attribute_name):
	Expression(ATTRIBUTE)
{
	this->attrib = to_attribute(attribute_name);
    this->selected = std::move(selected.get_unique());

	// set the type
	this->t = 
	DataType(
		INT,
		NONE,
		symbol_qualities(
			false,	// not const
			false,	// not static
			false,	// not dynamic
			false,	// not signed
			true	// is unsigned
		)	// not long, short, or extern
	);

	// all attributes are final; they are not necessarily known at compile time, but they are not directly modifiable
	this->t.get_qualities().add_quality(FINAL);
}

AttributeSelection::AttributeSelection(Binary &to_deconstruct): Expression(ATTRIBUTE)
{
	// Construct an 'AttributeSelection' object from a Binary expression
	
	// as long as we have a valid binary expression, continue
	if (to_deconstruct.get_right().get_expression_type() == KEYWORD_EXP) {
		this->selected = std::move(to_deconstruct.get_left().get_unique());
		auto right = static_cast<KeywordExpression&>(to_deconstruct.get_right());
		this->attrib = to_attribute(right.get_keyword());
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

std::vector<Expression*> ListExpression::get_list()
{
    std::vector<Expression*> to_return;
    for (auto it = this->list_members.begin(); it != this->list_members.end(); it++) {
        to_return.push_back(it->get());
    }
	return to_return;
}

std::unique_ptr<Expression> ListExpression::get_unique() {
    return std::make_unique<ListExpression>(*this);
}

ListExpression::ListExpression(std::vector<std::shared_ptr<Expression>> list_members, Type list_type) :
	Expression(LIST),
	list_members(list_members),
	primary(list_type)
{
}

ListExpression::ListExpression(): ListExpression({}, NONE) {
}

// Keyword Expressions -- necessary for some expressions

std::string KeywordExpression::get_keyword() {
	return this->keyword;
}

DataType &KeywordExpression::get_type() {
	return this->t;
}

std::unique_ptr<Expression> KeywordExpression::get_unique() {
    return std::make_unique<KeywordExpression>(*this);
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

Expression &Binary::get_left() {
	return *this->left_exp.get();
}

Expression &Binary::get_right() {
	return *this->right_exp.get();
}

exp_operator Binary::get_operator() {
	return this->op;
}

std::unique_ptr<Expression> Binary::get_unique() {
    return std::make_unique<Binary>(*this);
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

Expression &Unary::get_operand() {
	return *this->operand.get();
}

std::unique_ptr<Expression> Unary::get_unique() {
    return std::make_unique<Unary>(*this);
}

Unary::Unary(std::shared_ptr<Expression> operand, exp_operator op) : Expression(UNARY), operand(operand), op(op) {
}

Unary::Unary(): Expression(UNARY) {
	this->op = NO_OP;
}



// Parsing function calls

Expression &Procedure::get_func_name() {
    return *this->name.get();
}

ListExpression &Procedure::get_args() {
    return *this->args.get();
}

Expression &Procedure::get_arg(size_t arg_no) {
    return *this->args->get_list().at(arg_no);
}

size_t Procedure::get_num_args() {
    return this->args->get_list().size();
}

std::unique_ptr<Expression> Procedure::get_unique() {
    return std::make_unique<Procedure>(*this);
}

Procedure::Procedure(
    std::shared_ptr<Expression> proc_name, 
    std::shared_ptr<ListExpression> proc_args
):
    Expression(PROC_EXP),
    name(proc_name),
    args(proc_args)
{
}

Procedure::Procedure(std::shared_ptr<Expression> proc_name, ListExpression *proc_args): Expression(PROC_EXP) {
    this->name = proc_name;
    this->args = std::make_shared<ListExpression>(*proc_args);
}

Procedure::Procedure(): Expression(PROC_EXP)
{
    this->name = nullptr;
}

Expression &CallExpression::get_func_name() {
	return this->proc->get_func_name();
}

std::vector<Expression*> CallExpression::get_args() {
	return this->proc->get_args().get_list();
}

Expression &CallExpression::get_arg(size_t i) {
	return this->proc->get_arg(i);
}

size_t CallExpression::get_args_size() {
	return this->proc->get_num_args();
}

std::unique_ptr<Expression> CallExpression::get_unique() {
    return std::make_unique<CallExpression>(*this);
}

CallExpression::CallExpression(
	Procedure *proc
): 
	Expression(CALL_EXP)
{
    this->proc = std::make_shared<Procedure>(*proc);
}

CallExpression::CallExpression(): Expression(CALL_EXP)
{
}


Expression &Indexed::get_index_value()
{
	return *this->index_value.get();
}

Expression &Indexed::get_to_index()
{
	return *this->to_index.get();
}

std::unique_ptr<Expression> Indexed::get_unique() {
    return std::make_unique<Indexed>(*this);
}

Indexed::Indexed(std::shared_ptr<Expression> to_index, std::shared_ptr<Expression> index_value): Expression(INDEXED)
{
	this->to_index = to_index;
	this->index_value = index_value;
}

Indexed::Indexed(): Indexed(nullptr, nullptr)
{
}

Expression &Cast::get_exp() {
	return *this->to_cast.get();
}

DataType& Cast::get_new_type() {
	return this->new_type;
}

Cast::Cast(Expression &to_cast, DataType new_type): Expression(CAST) {
	this->to_cast = std::move(to_cast.get_unique());
	this->new_type = new_type;
}

Cast::Cast(Binary &b): Expression(CAST) {
	if (b.get_operator() == TYPECAST && b.get_right().get_expression_type() == KEYWORD_EXP) {
		auto &kw = static_cast<KeywordExpression&>(b.get_right());
		this->to_cast = std::move(b.get_left().get_unique());
		this->new_type = kw.get_type();
	}
	else {
		this->expression_type = EXPRESSION_GENERAL;
		this->to_cast = nullptr;
	}
}
