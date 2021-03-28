// Expression.cpp
// Implementation of the Expression class

#include "Expression.h"

bool is_literal(const lexeme_type candidate_type) {
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


bool Expression::is_const() const
{
	return this->_const;
}

exp_type Expression::get_expression_type() const
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

Expression::Expression(const exp_type expression_type) : expression_type(expression_type) {
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

const DataType& Literal::get_data_type() const {
	return this->type;
}

const std::string& Literal::get_value() const {
	return this->value;
}

void Literal::override_qualities(symbol_qualities sq) {
	// update the data type (for postfixed quality overrides)
	this->type.add_qualities(sq);	// todo: ensure the type override is valid
}

bool Literal::has_type_information() const {
	return true;
}

Literal::Literal(Type data_type, const std::string& value, Type subtype) : Expression(LITERAL), value(value) {
    // symbol qualities for our DataType object
    bool const_q = true;
    bool long_q = false;
    bool short_q = false;
    bool signed_q = true;

    // set our symbol qualities
    symbol_qualities qualities(const_q, false, false, signed_q, !signed_q, long_q, short_q);  // literals are always considered const
	this->type = DataType(data_type, subtype, qualities);
}

Literal::Literal(const DataType& t, const std::string& value): Expression(LITERAL) {
	this->type = t;
	this->value = value;
}

Literal::Literal(): Expression(LITERAL) {
	this->type = DataType();
}


const std::string& Identifier::getValue() const {
	return this->value;
}

void Identifier::setValue(const std::string& new_value) {
	this->value = new_value;
}

Identifier::Identifier(const std::string& value)
	: Expression(IDENTIFIER)
	, value(value) { }

Identifier::Identifier()
	: Identifier("") { }


// Attribute Selection

const Expression &AttributeSelection::get_selected() const {
	return *this->selected;
}

attribute AttributeSelection::get_attribute() const {
	return this->attrib;
}

const DataType &AttributeSelection::get_data_type() const {
	return this->t;
}

attribute AttributeSelection::to_attribute(const std::string& to_convert) {
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

bool AttributeSelection::is_attribute(const std::string& a) {
	return to_attribute(a) != NO_ATTRIBUTE;
}

AttributeSelection::AttributeSelection(AttributeSelection &old)
	: Expression(ATTRIBUTE)
	, selected(std::move(old.selected))
	, t(old.t)
	, attrib(old.attrib) { }

AttributeSelection::AttributeSelection(std::unique_ptr<Expression>&& selected, const std::string& attribute_name)
	: Expression(ATTRIBUTE)
	, attrib( to_attribute(attribute_name) )
	, selected( std::move(selected) )
	, t(
		DataType{
			INT,
			NONE,
			symbol_qualities(
				false,	// not const
				false,	// not static
				false,	// not dynamic
				false	// not signed
			)	// not long, short, or extern
		}
	)
{
	// all attributes are final; they are not necessarily known at compile time, but they are not directly modifiable
	this->t.get_qualities().add_quality(FINAL);
}

AttributeSelection::AttributeSelection(std::unique_ptr<Binary>&& to_deconstruct): Expression(ATTRIBUTE)
{
	// Construct an 'AttributeSelection' object from a Binary expression
	
	// as long as we have a valid binary expression, continue
	if (to_deconstruct->get_right().get_expression_type() == KEYWORD_EXP) {
		this->selected = std::move(to_deconstruct->get_left_unique());

		auto right = static_cast<const KeywordExpression&>(to_deconstruct->get_right());
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

AttributeSelection::AttributeSelection(std::unique_ptr<Expression>&& selected, attribute attrib, const DataType& t)
	: selected(std::move(selected))
	, attrib(attrib)
	, t(t) { }

// Lists

bool ListExpression::has_type_information() const {
	return true;
}

Type ListExpression::get_list_type() const {
	return this->primary;
}

std::vector<const Expression*> ListExpression::get_list() const
{
    std::vector<const Expression*> to_return;
    for (auto it = this->list_members.begin(); it != this->list_members.end(); it++) {
        to_return.push_back(it->get());
    }
	return to_return;
}

void ListExpression::add_item(std::unique_ptr<Expression> to_add, const size_t index) {
    if (index <= this->list_members.size()) {
        auto it = this->list_members.begin() + index;
        this->list_members.insert(
            it,
            std::move(to_add)
        );
    }
    else {
        throw std::out_of_range("ListExpression out of range");
    }
}

ListExpression::ListExpression(std::vector<std::unique_ptr<Expression>>& list_members, Type list_type)
	: Expression(LIST)
	, primary(list_type)
{
	for (auto it = list_members.begin(); it != list_members.end(); it++)
	{
		this->list_members.push_back(std::move(*it));
	}
}

ListExpression::ListExpression(std::unique_ptr<Expression> arg, Type list_type)
	: Expression(LIST)
	, primary(list_type)
{
	this->list_members.push_back(std::move(arg));
}

ListExpression::ListExpression()
	: Expression(LIST)
{
	this->primary = NONE;	
}

// Keyword Expressions -- necessary for some expressions

const std::string& KeywordExpression::get_keyword() const {
	return this->keyword;
}

const DataType &KeywordExpression::get_type() const {
	return this->t;
}

bool KeywordExpression::has_type_information() const {
    return true;
}

void KeywordExpression::override_qualities(symbol_qualities sq) {
    this->t.add_qualities(sq);
}

KeywordExpression::KeywordExpression(const std::string& keyword):
	Expression(KEYWORD_EXP),
	keyword(keyword)
{
}

KeywordExpression::KeywordExpression(const DataType& t):
	KeywordExpression("")
{
	this->t = t;
}

KeywordExpression::KeywordExpression(const DataType& t, const std::string& keyword)
	: Expression(KEYWORD_EXP)
	, t(t)
	, keyword(keyword) { }

std::unique_ptr<Expression> Binary::get_left_unique()
{
	return std::move(this->left_exp);
}

std::unique_ptr<Expression> Binary::get_right_unique()
{
	return std::move(this->right_exp);
}

const Expression &Binary::get_left() const {
	return *this->left_exp.get();
}

const Expression &Binary::get_right() const {
	return *this->right_exp.get();
}

exp_operator Binary::get_operator() const {
	return this->op;
}

Binary::Binary(
	std::unique_ptr<Expression> left_exp,
	std::unique_ptr<Expression> right_exp,
	const exp_operator op
):
	Expression(BINARY),
	left_exp(std::move(left_exp)),
	right_exp(std::move(right_exp)),
	op(op)
{
}

Binary::Binary(): Expression(BINARY) {
	Binary::op = NO_OP;	// initialized to no_op so we don't ever run into uninitialized variables
}



exp_operator Unary::get_operator() const {
	return this->op;
}

const Expression &Unary::get_operand() const {
	return *this->operand.get();
}

Unary::Unary(std::unique_ptr<Expression> operand, exp_operator op)
	: Expression(UNARY)
	, operand(std::move(operand))
	, op(op) { }

Unary::Unary(): Expression(UNARY) {
	this->op = NO_OP;
}



// Parsing function calls

const Expression &Procedure::get_func_name() const {
    return *this->name.get();
}

const ListExpression &Procedure::get_args() const {
    return dynamic_cast<ListExpression&>(*args);
}

const Expression &Procedure::get_arg(size_t arg_no) const {
    return *dynamic_cast<ListExpression*>(args.get())->get_list().at(arg_no);
}

size_t Procedure::get_num_args() const {
    return dynamic_cast<ListExpression*>(args.get())->get_list().size();
}

void Procedure::insert_arg(std::unique_ptr<Expression> to_insert, const size_t index) {
    dynamic_cast<ListExpression*>(args.get())->add_item(std::move(to_insert), index);
}

Procedure::Procedure(Procedure& other)
	: Expression(PROC_EXP)
	, name(std::move(other.name))
	, args(std::move(other.args))
{
}

Procedure::Procedure(std::unique_ptr<Expression> proc_name, std::unique_ptr<Expression> proc_args)
	: Expression(PROC_EXP)
	, name(std::move(proc_name))
	, args(std::move(proc_args)) { }

Procedure::Procedure()
	: Expression(PROC_EXP)
	, name(nullptr)
	, args(nullptr) { }

/*
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
*/

/*
void CallExpression::insert_arg(Expression &to_insert, size_t index) {
    this->proc->insert_arg(to_insert, index);
}
*/

CallExpression::CallExpression(
	Procedure *proc
): 
    Procedure(*proc)
{
    this->expression_type = CALL_EXP;
}

CallExpression::CallExpression(CallExpression& other)
	: Procedure(other)
{
	this->expression_type = CALL_EXP;
}

CallExpression::CallExpression()
{
    this->expression_type = CALL_EXP;
}


const Expression &Indexed::get_index_value() const
{
	return *this->index_value.get();
}

const Expression &Indexed::get_to_index() const
{
	return *this->to_index.get();
}

Indexed::Indexed(std::unique_ptr<Expression> to_index, std::unique_ptr<Expression> index_value)
	: Expression(INDEXED)
	, to_index(std::move(to_index))
	, index_value(std::move(index_value)) { }

Indexed::Indexed(): Indexed(nullptr, nullptr)
{
}

const Expression &Cast::get_exp() const {
	return *this->to_cast;
}

const DataType& Cast::get_new_type() const {
	return this->new_type;
}

Cast::Cast(Cast &old): Expression(CAST) {
    this->new_type = old.new_type;
    this->to_cast = std::move(old.to_cast);
}

Cast::Cast(std::unique_ptr<Expression> to_cast, const DataType& new_type)
	: Expression(CAST)
	, to_cast(std::move(to_cast))
	, new_type(new_type) { }

Cast::Cast(std::unique_ptr<Binary> b): Expression(CAST) {
	if (b->get_operator() == TYPECAST && b->get_right().get_expression_type() == KEYWORD_EXP) {
		auto &kw = static_cast<const KeywordExpression&>(b->get_right());
		this->to_cast = std::move( b->get_left_unique() );
		this->new_type = kw.get_type();
	}
	else {
		this->expression_type = EXPRESSION_GENERAL;
		this->to_cast = nullptr;
	}
}
