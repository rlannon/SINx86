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
	this->message = "**** Compiler Error " + std::to_string(this->code) + ": " + this->message + " (error occurred at or near line " + std::to_string(this->line) + ")";
}

IllegalOperationException::IllegalOperationException(unsigned int line):
	CompilerException("This operation is not allowed here", compiler_errors::ILLEGAL_OPERATION_ERROR, line)
{

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
    CompilerException("Symbol is out of scope", compiler_errors::OUT_OF_SCOPE_ERROR)
{
    // No body necessary
}

TypeException::TypeException(unsigned int line) :
    CompilerException("Types are not compatible", compiler_errors::TYPE_ERROR, line)
{
    // Exception should be used when types are incompatible
}

VoidException::VoidException(unsigned int line) :
    CompilerException("Void type cannot be used in expression of this type", compiler_errors::VOID_TYPE_ERROR, line)
{
    // Exception should be used when 'void' type was found, but cannot be used here
}

ConstAssignmentException::ConstAssignmentException(unsigned int line) :
	CompilerException("Cannot make assignment to const-qualified variable", compiler_errors::CONST_ASSIGNMENT_ERROR, line)
{
	// we don't need a function body because we called the super constructor
}

// Compiler Warning

void compiler_warning(std::string message, unsigned int line_number) {
	std::cout << "**** Compiler Warning: " << message << " (line " << line_number << ")" << std::endl;
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
	message_ = "**** Parser Error " + std::to_string(code_) + ": " + message_ + " (line " + std::to_string(line_) + ")";
}

// Parser Exception -- missing semicolon error

MissingSemicolonError::MissingSemicolonError(const unsigned int& line) : ParserException("Syntax error; expected ';'", 0, line) {

}



// SINVM Exceptions

const char* VMException::what() const noexcept {
	return message.c_str();
}

VMException::VMException(const std::string& message, const uint16_t& address, const uint16_t& status) : message(message), address(address), status(status) {
	// we must construct the message here, in the constructor
	std::stringstream err_ss;
	std::bitset<8> bin_representation(status);
	err_ss << "**** SINVM Error: " << this->message << std::endl << "Error was encountered at memory location 0x" << std::hex << this->address << std::endl << "STATUS register was " << bin_representation << std::dec << std::endl;
	this->message = err_ss.str();
}


// SymbolTable Exceptions

const char* SymbolTableException::what() const noexcept {
	return message.c_str();
}

SymbolTableException::SymbolTableException(const std::string& message, const unsigned int& line) {
	// construct the message
	std::string err_message = "**** SymbolTable Error: " + message;
	// if we have line number data, append it to the error message
	if (line > 0) {
		err_message += " (line " + std::to_string(line);
	}
	this->line = line;
	this->message = err_message;
}


// Assembler Exceptions

const char* AssemblerException::what() const noexcept {
	return message.c_str();
}

AssemblerException::AssemblerException(const std::string& message, const unsigned int& line) {
	std::string err_message = "**** Assembler Error: " + message + " (line " + std::to_string(line) + ")";
	this->line = line;
	this->message = err_message;
}
