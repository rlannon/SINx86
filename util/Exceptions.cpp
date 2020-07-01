/*

SIN Toolchain
Exceptions.cpp
Copyright 2019 Riley Lannon

The implementation of the classes/methods defined in Exceptions.h

Note that all what() messages MUST be constructed in the constructor for the exception (unless we have a const char that does not rely on any variable data).

*/

#include "Exceptions.h"


// Compiler Exceptions

const char* CompilerException::what() const noexcept {
	return message.c_str();
}

CompilerException::CompilerException(const std::string& message, unsigned int code, unsigned int line) : message(message), code(code), line(line) {
	this->message = "**** Compiler error C" + std::to_string(this->code) + ": " + this->message + " (error occurred at or near line " + std::to_string(this->line) + ")";
}

InvisibleSymbolException::InvisibleSymbolException(unsigned int line):
	CompilerException(
		"Attempt to include a non-globalized symbol in SIN file; use Declarative SIN or \"extern\"",
		compiler_errors::INVISIBLE_SYMBOL,
		line
	)
{
	// super called
}

IllegalOperationException::IllegalOperationException(unsigned int line):
	CompilerException("This operation is not allowed here", compiler_errors::ILLEGAL_OPERATION_ERROR, line)
{
	// super called
}

IllegalReturnException::IllegalReturnException(unsigned int line):
CompilerException(
	"Return statement not allowed here; they may only occur inside functions",
	compiler_errors::ILLEGAL_RETURN_ERROR,
	line
) {
	// super called
}

NoReturnException::NoReturnException(unsigned int line) :
	CompilerException(
		"Return statement not found in function (perhaps not all control paths return a value?)",
		compiler_errors::NO_RETURN_ERROR,
		line
	)
{
	// super called
}

StructDefinitionException::StructDefinitionException(unsigned int line):
	CompilerException("Illegal; struct definitions may only include allocations", compiler_errors::ILLEGAL_OPERATION_ERROR, line)
{
	
}

SymbolNotFoundException::SymbolNotFoundException(unsigned int line) :
	CompilerException("Could not find referenced symbol", compiler_errors::SYMBOL_NOT_FOUND_ERROR, line)
{
	// we don't need a function body because we called the super constructor
}

DuplicateSymbolException::DuplicateSymbolException(unsigned int line) :
	CompilerException("Symbol already defined in this scope", compiler_errors::DUPLICATE_SYMBOL_ERROR, line)
{
	// function body not necessary (super called)
}

DuplicateDefinitionException::DuplicateDefinitionException(unsigned int line) :
	CompilerException("Definition for this resource (struct/function) already found", compiler_errors::DUPLICATE_DEFINITION_ERROR, line)
{
	// super called
}

UndefinedException::UndefinedException(unsigned int line) :
	CompilerException("Undefined reference to object", compiler_errors::UNDEFINED_ERROR, line)
{
	// function body not necessary (super called)
}

FunctionSignatureException::FunctionSignatureException(unsigned int line) :
	CompilerException("Function call does not match function signature", compiler_errors::SIGNATURE_ERROR, line)
{
	// body not necessary (super called)
}

InvalidSymbolException::InvalidSymbolException(unsigned int line) :
    CompilerException("Found a symbol, but it was of the improper type (e.g., VARIABLE instead of FUNCTION)", compiler_errors::INVALID_SYMBOL_TYPE_ERROR, line)
{
    /*
    Since all symbols use the same table, we can't have conflicting names between functions and variables.
    If a symbol is found, but the symbol is not a function, then we have an error.
    */
}

UnexpectedFunctionException::UnexpectedFunctionException(unsigned int line):
    CompilerException("Function name is not appropriate here without a call operator (@)", compiler_errors::UNEXPECTED_FUNCTION_SYMBOL, line)
{
    /*
    
    To be used when a function symbol is found, but it is not appropriate.
    For example, when evaluating an lvalue, if we see a function symbol without a call, then it should generate this error

    todo: introduce callable types that allow for this

    */
}

OutOfScopeException::OutOfScopeException(unsigned int line) :
    CompilerException("Symbol is out of scope", compiler_errors::OUT_OF_SCOPE_ERROR, line)
{
    // No body necessary
}

DeclarationException::DeclarationException(unsigned int line) :
CompilerException(
	"Declarations must be made in the global scope",
	compiler_errors::DECLARATION_ERROR,
	line
) {

}

TypeException::TypeException(unsigned int line) :
    CompilerException("Types are not compatible", compiler_errors::TYPE_ERROR, line)
{
    // Exception should be used when types are incompatible
}

ReturnMismatchException::ReturnMismatchException(unsigned int line):
CompilerException(
	"Return value does not match function signature",
	compiler_errors::RETURN_MISMATCH_ERROR,
	line
) {
	// super called
}

QualityConflictException::QualityConflictException(std::string &conflicting_quality, unsigned int line) :
	CompilerException(
		(
			"Symbol quality '" + conflicting_quality + "' may not be used here (there is a conflicting quality present)"),
			compiler_errors::QUALITY_CONFLICT_ERROR, 
			line
		)
{
	// Body not necessary (super called)
}

IllegalQualityException::IllegalQualityException(std::string &offending_quality, unsigned int &line) :
CompilerException(
	("Illegal symbol quality '" + offending_quality + "'"),
	compiler_errors::ILLEGAL_QUALITY_ERROR,
	line
) {
	// super called
}

TypeValidityViolation::TypeValidityViolation(unsigned int line) :
	CompilerException(
		"Type was parsed correctly, but violates SIN's type validity policy",
		compiler_errors::TYPE_VALIDITY_RULE_VIOLATION_ERROR,
		line
	)
{
	// super called
}

VariabilityPolicyViolation::VariabilityPolicyViolation(unsigned int line) :
	CompilerException(
		"Type variability policy violation",
		compiler_errors::VARIABILITY_ERROR,
		line
	) {
	// super called
}

TypeDemotionException::TypeDemotionException(unsigned int line) :
	CompilerException(
		"Cannot demote right-hand type to less restrictive variability quality",
		compiler_errors::VARIABILITY_ERROR,
		line
) {
	// super called
}

VoidException::VoidException(unsigned int line) :
    CompilerException("Void type cannot be used in expression of this type", compiler_errors::VOID_TYPE_ERROR, line)
{
    // Exception should be used when 'void' type was found, but cannot be used here
}

OperatorTypeError::OperatorTypeError(std::string op, std::string type, unsigned int line) :
CompilerException(
	("Operator '" + op + "' may not be used on expressions of type '" + type + "'"),
	compiler_errors::OPERATOR_TYPE_ERROR,
	line
) {
	// super called
}

IllegalUnaryOperatorError::IllegalUnaryOperatorError(unsigned int line) :
CompilerException(
	"Invalid unary operator",
	compiler_errors::INVALID_UNARY_OPERATOR_ERROR,
	line
) {
	// super called
}

UnaryTypeNotSupportedError::UnaryTypeNotSupportedError(unsigned int line) :
CompilerException(
	"This unary operator may not be used on expressions of this type",
	compiler_errors::UNARY_TYPE_NOT_SUPPORTED,
	line
) {
	// super called
}

ConstAssignmentException::ConstAssignmentException(unsigned int line) :
	CompilerException("Cannot make assignment to const-qualified variable", compiler_errors::CONST_ASSIGNMENT_ERROR, line)
{
	// we don't need a function body because we called the super constructor
}

FinalAssignmentException::FinalAssignmentException(unsigned int line) :
CompilerException(
	"Cannot make assignment to initialized final-qualified variable",
	compiler_errors::FINAL_ASSIGNMENT_ERROR,
	line
) {
	// super called
}

ConstAllocationException::ConstAllocationException(unsigned int line) :
	CompilerException(
		"Constants must be initialized in their allocation",
		compiler_errors::CONST_ALLOCATION_ERROR,
		line
	) {
	// super called
}

InvalidTypecastException::InvalidTypecastException(unsigned int line) :
	CompilerException(
		"Illegal typecast",
		compiler_errors::INVALID_CAST_ERROR,
		line
	)
{
		// super called
}


// Warnings and notes

void compiler_warning(std::string message, unsigned int code, unsigned int line_number) {
	std::cout << "**** Compiler Warning W" << code << ": " << message << " (at or near line " << line_number << ")" << std::endl;
}

void compiler_note(std::string message, unsigned int line_number) {
	std::cout << "**** Note: " << message << " (line " << line_number << ")" << std::endl;
}

void parser_warning(std::string message, unsigned int line_number)
{
	std::cout << "**** Parser Warning: " << message << " (line " << line_number << ")" << std::endl;
}


// Parser Exceptions

const char* ParserException::what() const noexcept {
	return message_.c_str();
}

ParserException::ParserException(const std::string& message, const unsigned int& code, const unsigned int& line) : message_(message), code_(code), line_(line) {
	message_ = "**** Compiler error E" + std::to_string(code_) + ": " + message_ + " (line " + std::to_string(line_) + ")";
}

InvalidTokenException::InvalidTokenException(
	std::string offending_token,
	unsigned int line
) : ParserException(
	("Invalid token '" + offending_token + "' found while parsing"),
	compiler_errors::INVALID_TOKEN,
	line
) {
	// super called
}

IncompleteTypeError::IncompleteTypeError(const unsigned int &line):
	ParserException(
		"Incomplete type information",
		compiler_errors::INCOMPLETE_TYPE_ERROR,
		line
	)
{
	// super called
}

MissingSemicolonError::MissingSemicolonError(const unsigned int& line) : ParserException("Syntax error; expected ';'", compiler_errors::MISSING_SEMICOLON_ERROR, line) {

}

MissingIdentifierError::MissingIdentifierError(const unsigned int &line) :
ParserException(
	"Expected identifier",
	compiler_errors::MISSING_IDENTIFIER_ERROR,
	line
) {
	// super called
}

UnexpectedKeywordError::UnexpectedKeywordError(std::string &offending_keyword, const unsigned int &line) :
	ParserException(
		("Unexpected keyword '" + offending_keyword + "'"),
		compiler_errors::UNEXPECTED_KEYWORD_ERROR,
		line
	)
{
	// Used when a keyword is found, but one is not expected
}

CallError::CallError(const unsigned int &line):
	ParserException(
		"Expected parens enclosing arguments in function call",
		compiler_errors::MISSING_GROUPING_SYMBOL_ERROR,
		line
	)
{
	// super called
}

UndefinedOperatorError::UndefinedOperatorError(std::string op, unsigned int line) :
	CompilerException(
		"The " + op + " operator is undefined for this data type",
		line
	)
{
	// super called
}

NonModifiableLValueException::NonModifiableLValueException(unsigned int line) :
	CompilerException(
		"Left-hand side of assignment must be a modifiable-lvalue",
		compiler_errors::NON_MODIFIABLE_LVALUE_ERROR,
		line
	)
{
	// super called
}

ReferencedBeforeInitializationException::ReferencedBeforeInitializationException(std::string symbol_name, unsigned int line) :
	CompilerException(
		"Symbol '" + symbol_name + "' referenced before assignment",
		compiler_errors::REFERENCED_BEFORE_ASSIGNMENT_ERROR,
		line
	)
{
	// super called
}
