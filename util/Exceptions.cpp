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
	this->message = "**** Compiler Error " + std::to_string(this->code) + ": " + this->message + " (line " + std::to_string(this->line) + ")";
}

SymbolNotFoundException::SymbolNotFoundException(unsigned int line) :
	CompilerException("Could not find referenced symbol", compiler_errors::SYMBOL_NOT_FOUND_ERROR, line)
{
	// we don't need a function body because we called the super constructor
}

InvalidSymbolException::InvalidSymbolException(unsigned int line) :
    CompilerException("Found a symbol, but it was of the improper type (e.g., VARIABLE instead of FUNCTION)", compiler_errors::INVALID_SYMBOL_TYPE_ERROR, line)
{
    /*
    Since all symbols use the same table, we can't have conflicting names between functions and variables.
    If a symbol is found, but the symbol is not a function, then we have an error.
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
