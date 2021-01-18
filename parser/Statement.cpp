/*

SIN Toolchain
Statement.cpp
Copyright 2019 Riley Lannon

The implementation of the Statement parent class and its various child classes

*/


#include "Statement.h"
#include <utility>


stmt_type Statement::get_statement_type() const {
	return Statement::statement_type;
}

unsigned int Statement::get_line_number() const
{
	return this->line_number;
}

void Statement::set_line_number(unsigned int line_number)
{
	this->line_number = line_number;
}

Statement::Statement() {
	// create default scope names and levels
	// TODO: remove scope name and level in symbol?
	this->scope_level = 0;
	this->scope_name = "global";
}

Statement::Statement(const stmt_type statement_type) {
	this->statement_type = statement_type;
}

Statement::Statement(const stmt_type statement_type, const unsigned int line_number) : statement_type(statement_type), line_number(line_number) {
	
}

Statement::~Statement() {
}



/*******************	STATEMENT BLOCK CLASS	********************/


StatementBlock::StatementBlock() {
	this->statements_list = {};
	this->has_return = false;
}

StatementBlock::~StatementBlock() {
}

/*	Scoped Block	*/

const StatementBlock& ScopedBlock::get_statements() const {
	return this->statements;
}

ScopedBlock::ScopedBlock(StatementBlock statements): Statement(SCOPE_BLOCK) {
	this->statements = statements;
}


/*******************		INCLUDE CLASS		********************/


const std::string& Include::get_filename() const {
	return this->filename;
}

Include::Include(const std::string& filename) 
	: Statement(INCLUDE)
	, filename(filename) { }

Include::Include(): Include("") { }

/*******************		DECLARATION CLASS		********************/


std::string Declaration::get_name() const {
	return this->name;
}

DataType& Declaration::get_type_information() {
	return this->type;
}

const DataType& Declaration::get_type_information() const
{
	return this->type;
}

bool Declaration::is_function() const
{
	return this->function_definition;
}

bool Declaration::is_struct() const
{
	return this->struct_definition;
}

Expression *Declaration::get_initial_value()
{
	return this->initial_value.get();
}

std::vector<Statement*> Declaration::get_formal_parameters() {
	std::vector<Statement*> to_return;
    for (auto it = this->formal_parameters.begin(); it != this->formal_parameters.end(); it++) {
        to_return.push_back(it->get());
    }
    return to_return;
}

calling_convention Declaration::get_calling_convention() const {
	return this->call_con;
}

// Constructors
Declaration::Declaration(DataType type, std::string var_name, std::shared_ptr<Expression> initial_value, bool is_function, bool is_struct, std::vector<std::shared_ptr<Statement>> formal_parameters) :
	Statement(DECLARATION),
	type(type),
	name(var_name),
	initial_value(initial_value),
	function_definition(is_function),
	struct_definition(is_struct),
	formal_parameters(formal_parameters)
{
	this->call_con = SINCALL;
}

Declaration::Declaration():
Declaration(DataType(), "", nullptr, false, false, {}) {
	
}

/*******************	ALLOCATION CLASS	********************/


DataType& Allocation::get_type_information() {
	return this->type_information;
}

std::string Allocation::get_var_type_as_string(Type to_convert) {
	std::string types_list[4] = { "int", "float", "string", "bool" };
	Type _types[4] = { INT, FLOAT, STRING, BOOL };

	for (int i = 0; i < 4; i++) {
		if (to_convert == _types[i]) {
			return types_list[i];
		}
		else {
			continue;
		}
	}

	// if we get here, the type was not in the list
	return "[unknown type]";
}

const std::string& Allocation::get_name() const {
	return this->value;
}

bool Allocation::was_initialized() const
{
	return this->initialized;
}

Expression *Allocation::get_initial_value()
{
	return this->initial_value.get();
}

Allocation::Allocation(const DataType& type_information, const std::string& value, const bool initialized, std::unique_ptr<Expression> initial_value) :
	Statement(ALLOCATION),
	type_information(type_information),
	value(value),
	initialized(initialized),
	initial_value(std::move(initial_value))
{
}

Allocation::Allocation(): Statement(ALLOCATION) {
	Allocation::type_information = type_information;
	Allocation::initialized = false;
}



/*******************	ASSIGNMENT CLASS	********************/

const Expression & Assignment::get_lvalue() const {
	return *this->lvalue.get();
}

const Expression & Assignment::get_rvalue() const {
	return *this->rvalue_ptr.get();
}

Assignment::Assignment(std::unique_ptr<Expression> lvalue, std::unique_ptr<Expression> rvalue) : 
	Statement(ASSIGNMENT),
	lvalue(std::move(lvalue)), 
	rvalue_ptr(std::move(rvalue)) 
{
}

Assignment::Assignment(Identifier lvalue, std::unique_ptr<Expression> rvalue) : 
	Statement(ASSIGNMENT),
	rvalue_ptr(std::move(rvalue))
{
	this->lvalue = std::make_unique<Identifier>(lvalue);
}

Assignment::Assignment():
	Assignment(nullptr, nullptr)
{
}

// Movements

Movement::Movement(std::unique_ptr<Expression> lvalue, std::unique_ptr<Expression> rvalue) :
	Assignment(std::move(lvalue), std::move(rvalue))
{
	this->statement_type = MOVEMENT;	// since we call the assignment constructor, we need to override the statement type
}


/*******************	RETURN STATEMENT CLASS		********************/


const Expression & ReturnStatement::get_return_exp() const {
	return *this->return_exp.get();
}


ReturnStatement::ReturnStatement(std::unique_ptr<Expression> exp_ptr):
	Statement(RETURN_STATEMENT)
{
	ReturnStatement::return_exp = std::move(exp_ptr);
}

ReturnStatement::ReturnStatement():
	ReturnStatement(nullptr)
{
}



/*******************	ITE CLASS		********************/

const Expression &IfThenElse::get_condition() const {
	return *this->condition.get();
}

const Statement *IfThenElse::get_if_branch() const {
	return this->if_branch.get();
}

const Statement *IfThenElse::get_else_branch() const {
	return this->else_branch.get();
}

IfThenElse::IfThenElse(std::unique_ptr<Expression> condition_ptr, std::unique_ptr<Statement> if_branch_ptr, std::unique_ptr<Statement> else_branch_ptr)
	: Statement(IF_THEN_ELSE)
	, condition(std::move(condition_ptr))
	, if_branch(std::move(if_branch_ptr))
	, else_branch(std::move(else_branch_ptr))
{
}

IfThenElse::IfThenElse(std::unique_ptr<Expression> condition_ptr, std::unique_ptr<Statement> if_branch_ptr):
	IfThenElse(std::move(condition_ptr), std::move(if_branch_ptr), nullptr)
{
}

IfThenElse::IfThenElse():
	Statement(IF_THEN_ELSE)
{
}



/*******************	WHILE LOOP CLASS		********************/

const Expression &WhileLoop::get_condition() const
{
	return *this->condition.get();
}

const Statement *WhileLoop::get_branch() const
{
	return this->branch.get();
}

WhileLoop::WhileLoop(std::unique_ptr<Expression> condition, std::unique_ptr<Statement> branch) : 
	Statement(WHILE_LOOP),
	condition(std::move(condition)),
	branch(std::move(branch))
{
}

WhileLoop::WhileLoop(): Statement(WHILE_LOOP) {
}


/*******************	DEFINITION CLASS		********************/

const std::string& Definition::get_name() const {
	return this->name;
}

const StatementBlock &Definition::get_procedure() const {
	return *this->procedure.get();
}

Definition::Definition(const std::string& name, std::unique_ptr<StatementBlock> procedure)
	: Statement()
	, name(name)
	, procedure(std::move(procedure))
{
}

Definition::Definition() {

}

Definition::~Definition() {
	
}

/*******************	FUNCTION DEFINITION CLASS		********************/

const DataType &FunctionDefinition::get_type_information() const
{
	return this->return_type;
}

std::vector<const Statement*> FunctionDefinition::get_formal_parameters() const {
	std::vector<const Statement*> to_return;
    for (auto it = this->formal_parameters.begin(); it != this->formal_parameters.end(); it++) {
        to_return.push_back(it->get());
    }
    return to_return;
}

calling_convention FunctionDefinition::get_calling_convention() const {
	return this->call_con;
}

FunctionDefinition::FunctionDefinition(
	const std::string& name,
	const DataType& return_type,
	const std::vector<std::shared_ptr<Statement>>& args_ptr,
	std::unique_ptr<StatementBlock> procedure_ptr,
	const calling_convention call_con
)
	: Definition(name, std::move(procedure_ptr))
	, return_type(return_type)
	, formal_parameters(args_ptr)
	, call_con(call_con)
{
	this->statement_type = FUNCTION_DEFINITION;
}

FunctionDefinition::FunctionDefinition()
	: Definition()
{
	this->statement_type = FUNCTION_DEFINITION;
}


/*******************	STRUCT DEFINITION CLASS		********************/

StructDefinition::StructDefinition(const std::string& name, std::unique_ptr<StatementBlock> procedure_ptr):
	Definition(name, std::move(procedure_ptr))
{
	this->statement_type = STRUCT_DEFINITION;
}


/*******************	FUNCTION CALL CLASS		********************/
/*
Expression &Call::get_func_name() {
	return this->call_exp.get_func_name();
}

CallExpression &Call::get_call_expression() {
    return this->call_exp;
}

size_t Call::get_args_size() {
	return this->call_exp.get_args_size();
}

Expression &Call::get_arg(size_t index) {
	// return one argument
	return this->call_exp.get_arg(index);
}

std::vector<Expression*> Call::get_args() {
	// return all function arguments
	return this->call_exp.get_args();
}
*/
Call::Call(const CallExpression& call_exp): 
	Statement(CALL),
	CallExpression(call_exp)
{
}

Call::Call(): Statement(CALL) {
}


/*******************		INLINE ASM CLASS		********************/

const std::string& InlineAssembly::get_asm_code() const
{
	return this->asm_code;
}

InlineAssembly::InlineAssembly(const std::string& asm_code) : 
	Statement(INLINE_ASM),
	asm_code(asm_code)
{
}

InlineAssembly::InlineAssembly():
	InlineAssembly("")
{
}


/*******************		FREE MEMORY CLASS		********************/

const Expression &FreeMemory::get_freed_memory() const {
	return *this->to_free.get();
}

FreeMemory::FreeMemory(std::shared_ptr<Expression> to_free):
	Statement(FREE_MEMORY),
	to_free(to_free)
{
}

FreeMemory::FreeMemory(): Statement(FREE_MEMORY)
{
}
