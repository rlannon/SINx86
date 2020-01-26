/*

SIN Toolchain
Exceptions.h
Copyright 2019 Riley Lannon

A class to contain all of our custom exceptions.

*/

#pragma once

#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream>
#include <bitset>

#include "CompilerErrorCodes.h"

/*

	Exceptions.h
	Copyright 2019 Riley Lannon

	The purpose of the Exceptions files is to implement the various custom exceptions we use in the program.

*/

class CompilerException : public std::exception
{
protected:
	std::string message;
	unsigned int code;
	unsigned int line;
public:
	explicit CompilerException(const std::string& message, unsigned int code = 0, unsigned int line = 0);
	virtual const char* what() const noexcept;
};

class IllegalOperationException : public CompilerException
{
public:
	explicit IllegalOperationException(unsigned int line);
};

class IllegalReturnException : public CompilerException
{
public:
	explicit IllegalReturnException(unsigned int line);
};

class StructDefinitionException : public CompilerException
{
public:
	explicit StructDefinitionException(unsigned int line);
};

class SymbolNotFoundException : public CompilerException
{
public:
	explicit SymbolNotFoundException(unsigned int line);
};

class DuplicateSymbolException : public CompilerException
{
public:
	explicit DuplicateSymbolException(unsigned int line);
};

class UndefinedException : public CompilerException
{
public:
	explicit UndefinedException(unsigned int line);
};

class FunctionSignatureException : public CompilerException
{
public:
	explicit FunctionSignatureException(unsigned int line);
};

class InvalidSymbolException : public CompilerException
{
public:
    explicit InvalidSymbolException(unsigned int line);
};

class UnexpectedFunctionException : public CompilerException
{
public:
    explicit UnexpectedFunctionException(unsigned int line);
};

class OutOfScopeException : public CompilerException
{
public:
    explicit OutOfScopeException(unsigned int line);
};

class DeclarationException : public CompilerException
{
public:
	explicit DeclarationException(unsigned int line);
};

class TypeException : public CompilerException
{
public:
    explicit TypeException(unsigned int line);
};

class ReturnMismatchException : public CompilerException
{
public:
	explicit ReturnMismatchException(unsigned int line);
};

class QualityConflictException : public CompilerException
{
public:
	explicit QualityConflictException(std::string &conflicting_quality, unsigned int line);
};

class IllegalQualityException : public CompilerException
{
public:
	explicit IllegalQualityException(std::string &offending_quality, unsigned int &line);
};

class VoidException : public CompilerException
{
public:
    explicit VoidException(unsigned int line);
};

class OperatorTypeError : public CompilerException
{
public:
	explicit OperatorTypeError(std::string op, std::string type, unsigned int line);
};

class ConstAssignmentException : public CompilerException
{
public:
	explicit ConstAssignmentException(unsigned int line);
};

class FinalAssignmentException : public CompilerException
{
public:
	explicit FinalAssignmentException(unsigned int line);
};

// sometimes, we want to print an error message, but we don't need to stop compilation
void compiler_warning(std::string message, unsigned int line = 0);


class ParserException : public std::exception
{
protected:
	std::string message_;
	unsigned int code_;
	unsigned int line_;
public:
	explicit ParserException(const std::string& message, const unsigned int& code, const unsigned int& line);
	virtual const char* what() const noexcept;
};

class InvalidTokenException : public ParserException
{
public:
	explicit InvalidTokenException(std::string offending_token, unsigned int line);
};

class IncompleteTypeError : public ParserException
{
public:
	explicit IncompleteTypeError(const unsigned int &line);
};

class MissingSemicolonError : public ParserException
{
public:
	explicit MissingSemicolonError(const unsigned int &line);
};

class MissingIdentifierError : public ParserException
{
public:
	explicit MissingIdentifierError(const unsigned int &line);
};

class UnexpectedKeywordError : public ParserException
{
public:
	explicit UnexpectedKeywordError(std::string &offending_keyword, const unsigned int &line);
};

class CallError : public ParserException
{
public:
	explicit CallError(const unsigned int &line);
};

// like in the compiler, we sometimes want to print warnings without stopping parsing
void parser_warning(std::string message, unsigned int line_number = 0);
