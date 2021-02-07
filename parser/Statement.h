/*

SIN Toolchain
Statement.h
Copyright 2019 Riley Lannon

Contains the "Statement" class an its child classes. Such objects are generated by the Parser when creating the AST and used by the compiler to generate the appropriate assembly.

*/

#pragma once

#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <sstream>

#include "Expression.h"
#include "../util/EnumeratedTypes.h"
#include "../util/DataType.h"


// Statement is the base class for all statements

class Statement
{
protected:
	stmt_type statement_type;	// tells us whether its an Allocation, and Assignment, an ITE...
	std::string scope_name;	// to track the scope name under which the statement is being executed
	unsigned int scope_level;	// to track the scope level
	unsigned int line_number;	// the line number on which the first token of the statement can be found in the file

	// TODO: add scope information to statements in Parser

public:
	stmt_type get_statement_type() const;

	unsigned int get_line_number() const;
	void set_line_number(unsigned int line_number);

	Statement();
	Statement(const stmt_type statement_type);
	Statement(const stmt_type statement_type, const unsigned int line_number);
	virtual ~Statement();
};

class StatementBlock
{
public:
	std::vector<std::shared_ptr<Statement>> statements_list;
	bool has_return;	// for functions, a return statement is necessary; this will also help determine if all control paths have a return value

	StatementBlock();
	~StatementBlock();
};

class ScopedBlock: public Statement
{
	StatementBlock statements;
public:
	const StatementBlock& get_statements() const;
	ScopedBlock(const StatementBlock& statements);
};

class Include : public Statement
{
	std::string filename;
public:
	const std::string& get_filename() const;
	Include(const std::string& filename);
	Include();
};

class Declaration : public Statement
{
	/*
	
	When we want to add a symbol to our symbol table, but not include an implementation of that symbol, the 'decl' keyword is used; e.g.,
		decl int myInt;	<- declares an integer variable 'myInt'
		decl int length(alloc string to_get);	<- declares a function called 'length'
	
	This allows a program to add symbols to its table without the implementation of those symbols; they can be added to the executable file at link time.
	This is useful for compiled libraries, removing the requirement of compiling said library every time a project using it is compiled.
	
	*/

	DataType type;
	bool function_definition;	// whether it's the declaration of a function
	bool struct_definition;	// whether it's the declaration of a struct

	std::string name;

	std::unique_ptr<Expression> initial_value;

	std::vector<std::unique_ptr<Statement>> formal_parameters;
	calling_convention call_con;
public:
	const std::string& get_name() const;

	DataType& get_type_information();
	const DataType& get_type_information() const;

	bool is_function() const;
	bool is_struct() const;

	Expression *get_initial_value();

	std::vector<Statement*> get_formal_parameters();
	std::vector<const Statement*> get_formal_parameters() const;
	calling_convention get_calling_convention() const;

	Declaration(const DataType& type, const std::string& var_name, std::unique_ptr<Expression>&& initial_value = std::make_unique<Expression>(EXPRESSION_GENERAL), bool is_function = false, bool is_struct = false);
	Declaration(const DataType& type, const std::string& var_name, std::unique_ptr<Expression>&& initial_value, bool is_function, bool is_struct, std::vector<std::unique_ptr<Statement>>& formal_parameters);
	Declaration();
};

class Allocation : public Statement
{
	/*
	
	For a statement like:
		alloc int myInt;
	we create an allocation statement like so:
		type			:	INT
		value			:	myInt
		initialized		:	false
		initial_value	:	(none) 
		length			:	0

	We can also use what is called "alloc-assign syntax" in SIN:
		alloc int myInt: 5;
	which will allocate the variable and make an initial assignment. In this case, the allocation looks like:
		type			:	INT
		value			:	myInt
		initialized		:	true
		initial_value	:	5

	This "alloc-assign" syntax is required for all const-qualified data types

	*/
	
	DataType type_information;
	std::string value;

	// If we have an alloc-define statement, we will need:
	bool initialized;	// whether the variable was defined upon allocation

	Identifier struct_name;	// structs will require a name
	std::unique_ptr<Expression> initial_value;
public:
	DataType& get_type_information();
	const DataType& get_type_information() const;
	static std::string get_var_type_as_string(Type to_convert);
	const std::string& get_name() const;

	bool was_initialized() const;
	const Expression *get_initial_value() const;

	Allocation(const DataType& type_information, const std::string& value, const bool was_initialized = false, std::unique_ptr<Expression>&& initial_value = nullptr);	// use default parameters to allow us to use alloc-define syntax, but we don't have to
	Allocation();
};

class Assignment : public Statement
{
	std::unique_ptr<Expression> lvalue;
	std::unique_ptr<Expression> rvalue_ptr;
public:
	// get the variables / expressions themselves
	const Expression &get_lvalue() const;
	const Expression &get_rvalue() const;

	Assignment(std::unique_ptr<Expression>&& lvalue, std::unique_ptr<Expression>&& rvalue);
	Assignment(const Identifier& lvalue, std::unique_ptr<Expression>&& rvalue);
	Assignment();
};

class Movement : public Assignment
{
	// Similar to an assignment, but should be marked as a movement
public:
	Movement(std::unique_ptr<Expression>&& lvalue, std::unique_ptr<Expression>&& rvalue);
};

class ReturnStatement : public Statement
{
	std::unique_ptr<Expression> return_exp;
public:
	const Expression &get_return_exp() const;

	ReturnStatement(std::unique_ptr<Expression>&& exp_ptr);
	ReturnStatement();
};

class IfThenElse : public Statement
{
	std::unique_ptr<Expression> condition;
	std::unique_ptr<Statement> if_branch;	// branches may be single statements or scope blocks
	std::unique_ptr<Statement> else_branch;
public:
	const Expression &get_condition() const;
	const Statement *get_if_branch() const;
	const Statement *get_else_branch() const;

	IfThenElse(std::unique_ptr<Expression>&& condition_ptr, std::unique_ptr<Statement>&& if_branch_ptr, std::unique_ptr<Statement>&& else_branch_ptr);
	IfThenElse(std::unique_ptr<Expression>&& condition_ptr, std::unique_ptr<Statement>&& if_branch_ptr);
	IfThenElse();
};

class WhileLoop : public Statement
{
	std::unique_ptr<Expression> condition;
	std::unique_ptr<Statement> branch;
public:
	const Expression &get_condition() const;
	const Statement *get_branch() const;

	WhileLoop(std::unique_ptr<Expression>&& condition, std::unique_ptr<Statement>&& branch);
	WhileLoop();
};


/*

Our definitions

There are two types of definitions in SIN - function definitions and struct definitions. Both statement classes inherit from 'definition', which has a name and a procedure associated with it

*/

class Definition: public Statement
{
	// The parent class for definitions
protected:
	std::string name;
	std::unique_ptr<StatementBlock> procedure;
public:
	const std::string& get_name() const;
	const StatementBlock &get_procedure() const;

	Definition(const std::string& name, std::unique_ptr<StatementBlock>&& procedure);
	Definition();
	~Definition();
};

class FunctionDefinition : public Definition
{
	// arguments and return types are only used for function definitions, so they should be inaccessible to child classes
	std::vector<std::unique_ptr<Statement>> formal_parameters;
	DataType return_type;

	calling_convention call_con;
public:
	const DataType &get_type_information() const;
	std::vector<const Statement*> get_formal_parameters() const;
	calling_convention get_calling_convention() const;

	FunctionDefinition(
        const std::string& name,
        const DataType& return_type,
        std::vector<std::unique_ptr<Statement>>& args_ptr,
        std::unique_ptr<StatementBlock>&& procedure_ptr,
        const calling_convention call_con = SINCALL
    );
	FunctionDefinition();
};

class StructDefinition : public Definition
{
	// A class for our struct definitions
public:
	StructDefinition(const std::string& name, std::unique_ptr<StatementBlock>&& producedure_ptr);
	StructDefinition();
};

class Call : public Statement, public CallExpression
{
public:
	Call(CallExpression& call_exp);
	Call();
};

class InlineAssembly : public Statement
{
	std::string asm_code;
public:
	const std::string& get_asm_code() const;

	InlineAssembly(const std::string& asm_code);
	InlineAssembly();
};

class FreeMemory : public Statement
{
	std::unique_ptr<Expression> to_free;
public:
	const Expression &get_freed_memory() const;

	FreeMemory(std::unique_ptr<Expression>&& to_free);
	FreeMemory();
};
