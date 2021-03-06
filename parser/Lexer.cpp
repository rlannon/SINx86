﻿/*

SIN Toolchain
lexer.cpp
Copyright 2020 Riley Lannon

The implementation of the lexer

*/

#include "Lexer.h"

// The list of language keywords
const std::set<std::string> Lexer::keywords{
	"alloc", "and", "array", "as", "asm", "bool", "char", "const", 
	"constexpr", "c64", "decl", "def", "dynamic", "else", "extern", "final", "float", "free", "if", "include", "int", 
	"len", "let", "long", "move", "not", "null", "or", "pass", "private", "proc", "ptr", "public", "raw", "readonly", "realloc", 
	"return", "short", "signed", "sincall", "size",  "static", "string", "struct", "tuple", "typename", "unmanaged", "unsigned", "var", "void", 
	"while", "windows", "xor"
};

// Our regular expressions
const std::string Lexer::punc_exp = R"([',;\[\]\{\}\(\)])";	// expression for punctuation
const std::string Lexer::op_exp = R"([\.\+\-\*/%=\&\|\^<>\$\?!~@#:])";	// expression for operations
const std::string Lexer::id_exp = "[_0-9a-zA-Z]";	// expression for interior id letters
const std::string Lexer::bool_exp = "[(true)|(false)]";

// The map containing our operator strings
const std::unordered_map<std::string, exp_operator> Lexer::op_strings({
	{"->", RIGHT_ARROW},
	{"<-", LEFT_ARROW},
	{"+=", PLUS_EQUAL},
	{"-=", MINUS_EQUAL},
	{"*=", MULT_EQUAL},
	{"/=", DIV_EQUAL},
	{"%=", MOD_EQUAL},
	{"&=", AND_EQUAL},
	{"|=", OR_EQUAL},
	{"^=", XOR_EQUAL},
	{"<<=", LEFT_SHIFT_EQUAL},
	{">>=", RIGHT_SHIFT_EQUAL},
	{"+", PLUS},
	{"-", MINUS},
	{"*", MULT},
	{"/", DIV},
	{"%", MODULO},
	{"=", EQUAL},
	{"!=", NOT_EQUAL},
	{">", GREATER},
	{"<", LESS},
	{">=", GREATER_OR_EQUAL},
	{"<=", LESS_OR_EQUAL},
	{"&", BIT_AND},
	{"|", BIT_OR},
	{"^", BIT_XOR},
	{"~", BIT_NOT},
	{">>", RIGHT_SHIFT},
	{"<<", LEFT_SHIFT},
	{"and", AND},
	{"or", OR},
	{"xor", XOR},
	{"not", NOT},
	{"as", TYPECAST},
	{"$", ADDRESS},
	{"*", DEREFERENCE},
	{":", ATTRIBUTE_SELECTION},
	{".", DOT},
	{"[", INDEX},
	{"@", CONTROL_TRANSFER},
    {"(", PROC_OPERATOR},
	{"::", SCOPE_RESOLUTION}
});


// Our stream access and test functions

inline bool Lexer::eof() const {
	return this->stream->peek() == EOF;
}

char Lexer::peek() const {
	if (this->eof()) {
		return EOF;
	}
	else
	{
		char ch = this->stream->peek();
		if (ch == '\r')
		{
			ch = this->stream->get();
			if (this->stream->peek() == '\n')
			{
				return '\n';
			}
			else
			{
				this->stream->unget();
			}
		}

		return ch;
	}
}

char Lexer::next() {
	if (this->eof()) {
		return EOF;
	}
	else
	{
		char ch = this->stream->get();

		// allow CRLF endings, ignoring carriage return
		if (ch == '\r' && this->peek() == '\n')
		{
			ch = this->stream->get();
		}
		
		// increment the line number if we hit a newline character
		if (ch == '\n') {
			this->current_line += 1;
		}

		return ch;
	}
}

/*

Our equivalency functions.
These are used to test whether a character is of a certain type.

*/

bool Lexer::match_character(const char ch, const std::string &expression) {
	try {
        // todo: is there a more efficient way to test one character against a regex?
		return std::regex_match(std::string(1, ch), std::regex(expression));
	}
	catch (const std::regex_error &e) {
		std::cerr << "REGEX ERROR:" << std::endl << e.what() << std::endl;	// todo: reevaluate whether these functions need to be static
		return false;
	}
}

inline bool Lexer::is_whitespace(const char ch) {
	return match_character(ch, "[ \n\t\r]");
}

inline bool Lexer::is_newline(const char ch) {
	return (ch == '\n');
}

inline bool Lexer::is_not_newline(const char ch) {
	return !is_newline(ch);
}

inline bool Lexer::is_digit(const char ch) {
	return match_character(ch, "[0-9]");
}

inline bool Lexer::is_letter(const char ch) {
	return match_character(ch, "[a-zA-Z]");
}

inline bool Lexer::is_number(const char ch) {
	return match_character(ch, "[0-9\\._]");
}

inline bool Lexer::is_id_start(const char ch) {

	/*
    
    is_id_start
    
    Returns true if the character is the start of an ID
    That is, if it starts with a letter or an underscore
    
    */

	return match_character(ch, "[_a-zA-Z]");
}

inline bool Lexer::is_id(const char ch) {
	/*  Returns true if the character is a valid id character  */

	return match_character(ch, id_exp);
}

inline bool Lexer::is_punc(const char ch) {
	return match_character(ch, punc_exp);
}

inline bool Lexer::is_op_char(const char ch) {
	return match_character(ch, op_exp);
}

inline bool Lexer::is_boolean(const std::string &candidate) {
	return (candidate == "true" || candidate == "false");
}

inline bool Lexer::is_keyword(const std::string &candidate) {
	return (bool)keywords.count(candidate);
}

inline bool Lexer::is_valid_operator(const std::string &candidate) {
	// Checks whether the lexeme is a valid operator for maybe_binary
	return (bool)Lexer::op_strings.count(candidate);
}

/*

Our read functions.
These will read out data in the stream and return it as a string with proper formatting.

*/

std::string Lexer::read_while(const std::function<bool(const char)>& predicate) {
	/*

	read_while
	Reads characters in the stream

	Continues to read through characters while the predicate function returns true.
	This will return a string containing the characters read.

	*/

	std::string msg = "";

	this->peek();
	if (this->stream->eof()) {
		std::cout << "EOF reached" << std::endl;
		return msg;
	}

	while (!this->eof() && predicate(this->peek())) {
		char next_char = this->peek();
		if (!this->eof()) {
			msg += this->next();
		}
	}

	return msg;
}


std::string Lexer::read_operator() {
	/*

	read_operator
	Reads in a valid operator from the stream

	*/

	// the string containing the full operator
	std::string op_string;

	// get the first character
	char ch = this->peek();
	op_string = std::string(1, ch);

    // '!' by itself is not an operator; read one more char
    if (ch == '!') {
        this->next();
        op_string.push_back(this->peek());
    }

	while (this->is_valid_operator(op_string)) {
		this->next();
		ch = this->peek();
		op_string.append(1, ch);
	}

	// we read ahead one too many characters, to pop the last one back
	op_string.pop_back();
	return op_string;
}

/*

Reads the next character in the stream to determine what to do next.

*/

lexeme Lexer::read_next() {
	lexeme_type type = NULL_LEXEME;
	std::string value = "";
	lexeme next_lexeme;

    this->read_while(&this->is_whitespace);	// continue reading through any whitespace

	char ch = this->peek();	// peek to see if we are still within the file

	if (this->stream->eof()) {
		next_lexeme = lexeme(NULL_LEXEME, "", 0);	// return an empty tuple if we have reached the end of the file
		this->exit_flag = true;	// set our exit flag
		return next_lexeme;
	}

	// todo: overhaul comments (return null lexeme if we have a comment, or maybe just have some sort of iteration)

	if (ch == '/') {	// if we have a slash, we may have a comment
		ch = this->next();	// advance one character

		if (this->peek() == '/') {	// if the next character is a slash, it's a line comment
			bool is_comment = true;

			while (is_comment) {
				this->next();	// eat the comment character
				this->read_while(
					[&, this] (const char c) -> bool
					{
						return !this->is_newline(c);
					}
				);	// skip characters until we hit a newline character
				this->next();	// eat the newline character

				// if we have whitespace (e.g., a tab), skip it
				this->read_while(&this->is_whitespace);

				// get the next character
				ch = this->peek();

				if (this->stream->eof() || this->eof()) {	// check to make sure we haven't gone past the end of the file
					next_lexeme = lexeme(NULL_LEXEME, "", 0);	// if we are, set the exit flag return an empty tuple
					this->exit_flag = true;	// set our exit flag
					return next_lexeme;	// return the empty lexeme
				}
				else if (ch == '/') {	// if not, check to see if we have another comment
					ch = this->next();	// eat the potential comment character

					// check to see if the next character is a slash; if so, we have another comment
					if (this->peek() == '/') {
						is_comment = true;
					}
					// if it is not a comment
					else {
						// set is_comment to false to terminate the loop
						is_comment = false;
						// use "unget" so that the position is in the correct place and "peek" reveals a slash
						this->stream->unget();
						ch = this->peek();
					}
				}
				else {
					is_comment = false;
				}
			}
		}
		// if the next character is a star, we have a block comment
		else if (this->peek() == '*') {
			bool is_comment = true;

			// continue ignoring characters while we are still in the comment
			while (is_comment) {
				this->next();
				if (this->peek() == '*') {
					this->next();
					if (this->peek() == '/') {
						this->next();
						is_comment = false;

						// read through any whitespace
						this->read_while(&is_whitespace);

						// get the next character; if we immediately have another comment, re-loop
						ch = this->peek();
						if (ch == '/') {
							this->next();
							if (this->peek() == '*') {
								is_comment = true;
							}
						}
					}
				}
			}
		}
		// else, just treat it as an op_char
		else {
			// use "unget" to move back one place so "peek" reveals a slash
			this->stream->unget();
			ch = this->peek();
		}
	}

	// If ch is not the end of the file and it is also not null
	if (ch != EOF && ch != '\0') {
		// test our various data types
		if (ch == '"') {
			type = lexeme_type::STRING_LEX;
			value = this->read_string();
		}
		else if (ch == '\'') {
			type = lexeme_type::CHAR_LEX;
			value = this->read_char();
		}
		else if (this->is_id_start(ch)) {
			value = this->read_while(&this->is_id);
			if (this->is_keyword(value)) {
				type = KEYWORD_LEX;
			}
			else if (this->is_boolean(value)) {
				type = BOOL_LEX;
			}
			else {
				type = IDENTIFIER_LEX;
			}
		}
		else if (this->is_digit(ch)) {
			// todo: allow 0x and 0b prefixes -- check the next character here (currently only allows base 10)

			std::string num = this->read_while(&this->is_number);	// get the number

			// finally, iterate over the string and add all non-underscored characters to the value
			// this allows the string to be more readable for the programmer, but we don't want them in our end result
			type = INT_LEX;
			bool found_decimal = false;
			for (auto it = num.cbegin(); it != num.cend(); it++) {
				if (*it == '.') {
					// if we already found a decimal point, we want to throw an exception -- it's an invalid numeric literal
					if (found_decimal) {
						throw CompilerException("Invalid numeric literal", compiler_errors::BAD_LITERAL, current_line);
					}
					value.push_back(*it);
					found_decimal = true;
					type = FLOAT_LEX;
				}
				else if (*it != '_') {
					value.push_back(*it);
				}
			}
		}
		else if (this->is_punc(ch)) {
			type = PUNCTUATION;
			value = this->next();	// we only want to read one punctuation mark at a time, so do not use "read_while"; they are to be kept separate
		}
		else if (this->is_op_char(ch)) {
			type = OPERATOR;
			value = this->read_operator();
		}
		else if (this->is_newline(ch)) {	// if we encounter a newline character
			this->peek();
			if (this->stream->eof()) {
				type = NULL_LEXEME;
				value = "";
				this->exit_flag = true;
			}
			else {
				this->next();	// otherwise, continue by getting the next character
			}
		}
		else {	// if the character in the file is not recognized, print an error message and quit lexing
			throw LexerException("Unrecognized character!", position, ch);
			this->exit_flag = true;
		}

		next_lexeme = lexeme(type, value, this->current_line);	// create our lexeme with our information
		return next_lexeme;

	}
	// the following circumstances will return a lexeme of no type with the value of NULL, EOF, or nothing; all will say it occurred on line 0
	else if (ch == '\0') {	// if there is a NULL character
		std::cout << ch << "   (NULL)" << std::endl;
		std::cout << "ch == NULL; done." << std::endl;
		this->exit_flag = true;
		return lexeme(NULL_LEXEME, "NULL", 0);
	}
	else if (ch == EOF) {	// if the end of file was reached
		std::cout << ch << "   (EOF)" << std::endl;
		std::cout << "end of file reached." << std::endl;
		this->exit_flag = true;
		return lexeme(NULL_LEXEME, "EOF", 0);
	}
	else {
		throw LexerException("Unexpected lexeme", this->position, ch);
		return lexeme(NULL_LEXEME, "", 0);
	}
}

void Lexer::read_lexeme() {
	this->current_lexeme = this->read_next();
}

std::string Lexer::read_string() {
	std::string str = "";	// start with an empty string for the message
	this->next();	// skip the initial quote in the string

	bool escaped = false;	// initialized our "escaped" identifier to false
	bool string_done = false;

	while (!this->eof() && !string_done) {
		char ch = this->next();	// get the character

		if (escaped) {	// if we have escaped the character
			str += ch;
			escaped = false;
		}
		else if (ch == '\\') {	// if we have not escaped the character and it's a backslash
			escaped = true;	// escape the next one
			str += ch;
		}
		else if (!escaped && ch == '"') {	// if we have not escaped it and the character is a double quote
			string_done = true;	// we are done with the string
		}
		else {	// otherwise, if we haven't escaped it and we didn't need to,
			str += ch;	// add it to the string
		}
	}

	return str;
}

std::string Lexer::read_char() {
	/*

	read_char
	Reads a char literal

	SIN char literals are structured like C characters; any ASCII character (hopefully UTF-8 support will be added someday) enclosed between single quotes.
	Since these characters may be escaped, they can be up to two C chars inside the single quotes; e.g., '\0'.
	In SIN, an empty char ('') is equivalent to '\0'.
	
	Note that here we will be giving the string representation of the character. So a newline will read as \ character, n character, not the actual newline character. This is because we are going to be supplying this information to NASM, so we want the string itself.

	*/

	std::string to_return = "";
	this->next();
	
	while (!this->eof() && this->peek() != '\'') {
		to_return.append(
			1, this->next()
		);
	}

	// if we had '', it should be interpreted as null
	if (to_return.length() == 0) {
		to_return = "\\0";
	}

	// eat the ' character
	this->next();

	return to_return;
}

std::string Lexer::read_ident() {
	std::string identifier = "";

	identifier += this->read_while(&this->is_id);

	return identifier;
}

// A function to check whether our exit flag is set or not
bool Lexer::exit_flag_is_set() const {
	return exit_flag;
}

// allow a lexeme to be written to an ostream

std::ostream& Lexer::write(std::ostream& os) const {
	return os << "{ \"" << this->current_lexeme.type << "\" : \"" << this->current_lexeme.value << "\" }";
}

// add a stream to be lexed
void Lexer::add_file(std::istream &input) {
	this->stream = &input;
	this->position = 0;
	this->current_line = 1;
	this->exit_flag = false;
}

// Constructor and Destructor

Lexer::Lexer(std::istream& input)
{
	Lexer::stream = &input;
	Lexer::position = 0;
	Lexer::current_line = 1;
	Lexer::exit_flag = false;
}

Lexer::Lexer() {
	
}


Lexer::~Lexer()
{
}

std::ostream& operator<<(std::ostream& os, const Lexer& lexer) {
	return lexer.write(os);
}



LexerException::LexerException(const std::string& err_message, const int err_position, const char& ch) : message_(err_message), position_(err_position), ch_(ch) {

}

const char* LexerException::what() const noexcept {
	return LexerException::message_.c_str();
}

char LexerException::get_char() {
	return ch_;
}

int LexerException::get_pos() {
	return position_;
}
