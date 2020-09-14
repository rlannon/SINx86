/*

SIN Toolchain
Statement.cpp
Copyright 2019 Riley Lannon

The implementation of the Statement parent class and its various child classes

*/


#include "Statement.h"


stmt_type Statement::get_statement_type() {
	return Statement::statement_type;
}

unsigned int Statement::get_line_number()
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

Statement::Statement(stmt_type statement_type) {
	this->statement_type = statement_type;
}

Statement::Statement(stmt_type statement_type, unsigned int line_number) : statement_type(statement_type), line_number(line_number) {
	
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

StatementBlock ScopedBlock::get_statements() {
	return this->statements;
}

ScopedBlock::ScopedBlock(StatementBlock statements): Statement(SCOPE_BLOCK) {
	this->statements = statements;
}


/*******************		INCLUDE CLASS		********************/


std::string Include::get_filename() {
	return this->filename;
}

Include::Include(std::string filename) : Statement(INCLUDE), filename(filename) {
}

Include::Include(): Include("") {
}

/*******************		DECLARATION CLASS		********************/


std::string Declaration::get_name() const {
	return this->name;
}

DataType& Declaration::get_type_information() {
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

std::string Allocation::get_name() {
	return this->value;
}

bool Allocation::was_initialized()
{
	return this->initialized;
}

Expression *Allocation::get_initial_value()
{
	return this->initial_value.get();
}

Allocation::Allocation(DataType type_information, std::string value, bool initialized, std::shared_ptr<Expression> initial_value) :
	Statement(ALLOCATION),
	type_information(type_information),
	value(value),
	initialized(initialized),
	initial_value(initial_value)
{
}

Allocation::Allocation(): Statement(ALLOCATION) {
	Allocation::type_information = type_information;
	Allocation::initialized = false;
}



/*******************	ASSIGNMENT CLASS	********************/

Expression & Assignment::get_lvalue() {
	return *this->lvalue.get();
}

Expression & Assignment::get_rvalue() {
	return *this->rvalue_ptr.get();
}

Assignment::Assignment(std::shared_ptr<Expression> lvalue, std::shared_ptr<Expression> rvalue) : 
	Statement(ASSIGNMENT),
	lvalue(lvalue), 
	rvalue_ptr(rvalue) 
{
}

Assignment::Assignment(Identifier lvalue, std::shared_ptr<Expression> rvalue) : 
	Statement(ASSIGNMENT),
	rvalue_ptr(rvalue)
{
	this->lvalue = std::make_shared<Identifier>(lvalue);
}

Assignment::Assignment():
	Assignment(nullptr, nullptr)
{
}

// Movements

Movement::Movement(std::shared_ptr<Expression> lvalue, std::shared_ptr<Expression> rvalue) :
	Assignment(lvalue, rvalue)
{
	this->statement_type = MOVEMENT;	// since we call the assignment constructor, we need to override the statement type
}


/*******************	RETURN STATEMENT CLASS		********************/


Expression & ReturnStatement::get_return_exp() {
	return *this->return_exp.get();
}


ReturnStatement::ReturnStatement(std::shared_ptr<Expression> exp_ptr):
	Statement(RETURN_STATEMENT)
{
	ReturnStatement::return_exp = exp_ptr;
}

ReturnStatement::ReturnStatement():
	ReturnStatement(nullptr)
{
}



/*******************	ITE CLASS		********************/

Expression &IfThenElse::get_condition() {
	return *this->condition.get();
}

Statement *IfThenElse::get_if_branch() {
	return this->if_branch.get();
}

Statement *IfThenElse::get_else_branch() {
	return this->else_branch.get();
}

IfThenElse::IfThenElse(std::shared_ptr<Expression> condition_ptr, std::shared_ptr<Statement> if_branch_ptr, std::shared_ptr<Statement> else_branch_ptr):
	Statement(IF_THEN_ELSE)
{
	IfThenElse::condition = condition_ptr;
	IfThenElse::if_branch = if_branch_ptr;
	IfThenElse::else_branch = else_branch_ptr;
}

IfThenElse::IfThenElse(std::shared_ptr<Expression> condition_ptr, std::shared_ptr<Statement> if_branch_ptr):
	IfThenElse(condition_ptr, if_branch_ptr, nullptr)
{
}

IfThenElse::IfThenElse():
	Statement(IF_THEN_ELSE)
{
}



/*******************	WHILE LOOP CLASS		********************/

Expression &WhileLoop::get_condition()
{
	return *this->condition.get();
}

Statement *WhileLoop::get_branch()
{
	return this->branch.get();
}

WhileLoop::WhileLoop(std::shared_ptr<Expression> condition, std::shared_ptr<Statement> branch) : 
	Statement(WHILE_LOOP),
	condition(condition),
	branch(branch)
{
}

WhileLoop::WhileLoop(): Statement(WHILE_LOOP) {
}


/*******************	DEFINITION CLASS		********************/

std::string Definition::get_name() {
	return this->name;
}

StatementBlock &Definition::get_procedure() {
	return *this->procedure.get();
}

Definition::Definition(std::string name, std::shared_ptr<StatementBlock> procedure):
	Statement(),
	name(name),
	procedure(procedure)
{
	this->name = name;
	this->procedure = procedure;
}

Definition::Definition() {

}

Definition::~Definition() {
	
}

/*******************	FUNCTION DEFINITION CLASS		********************/

DataType &FunctionDefinition::get_type_information()
{
	return this->return_type;
}

std::vector<Statement*> FunctionDefinition::get_formal_parameters() {
	std::vector<Statement*> to_return;
    for (auto it = this->formal_parameters.begin(); it != this->formal_parameters.end(); it++) {
        to_return.push_back(it->get());
    }
    return to_return;
}

calling_convention FunctionDefinition::get_calling_convention() {
	return this->call_con;
}

FunctionDefinition::FunctionDefinition(std::string name, DataType return_type, std::vector<std::shared_ptr<Statement>> args_ptr, std::shared_ptr<StatementBlock> procedure_ptr, calling_convention call_con):
	Definition(name, procedure_ptr),
	return_type(return_type),
	formal_parameters(args_ptr),
	call_con(call_con)
{
	FunctionDefinition::statement_type = FUNCTION_DEFINITION;
}

FunctionDefinition::FunctionDefinition() {
	FunctionDefinition::statement_type = FUNCTION_DEFINITION;
}


/*******************	STRUCT DEFINITION CLASS		********************/

StructDefinition::StructDefinition(std::string name, std::shared_ptr<StatementBlock> procedure_ptr):
	Definition(name, procedure_ptr)
{
	this->statement_type = STRUCT_DEFINITION;
}


/*******************	FUNCTION CALL CLASS		********************/

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

Call::Call(CallExpression call_exp): 
	Statement(CALL),
	call_exp(call_exp)
{
}

Call::Call(): Statement(CALL) {
}


/*******************		INLINE ASM CLASS		********************/

std::string InlineAssembly::get_asm_code()
{
	return this->asm_code;
}

InlineAssembly::InlineAssembly(std::string asm_code) : 
	Statement(INLINE_ASM),
	asm_code(asm_code)
{
}

InlineAssembly::InlineAssembly():
	InlineAssembly("")
{
}


/*******************		FREE MEMORY CLASS		********************/

Expression &FreeMemory::get_freed_memory() {
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
