/*

SIN Toolchain
Copyright 2019 Riley Lannon
Lexer.h

The Lexer class is used to handle the stream of input that we wish to parse. It takes a stream of input and returns tokens, each with a type and a value.
A lexeme may be returned from the stream using the read_next() function.

Note that the Lexer class does /not/ parse source files; it simply puts those files in a format that is usable by the language's parser, which is contained within the Parser class.

*/

#pragma once

#include <string>
#include <tuple>
#include <iostream>
#include <fstream>
#include <regex>
#include <functional>
#include <algorithm>
#include <vector>
#include <set>
#include <exception>
#include <unordered_map>

#include "lexeme.h"
#include "../util/Exceptions.h"

class Lexer
{
	std::istream* stream;
	int position;
	bool exit_flag;

	lexeme current_lexeme;
	unsigned int current_line;	// track what line we are on in the file

	static const std::set<std::string> keywords;	// our keywords

	// Strings for our longer/more complex regular expressions; this will make it easier to edit them
	static const std::string punc_exp;
	static const std::string op_exp;
	static const std::string id_exp;
	static const std::string bool_exp;

	// character access functions
	char peek() const;
	char next();

	// match a single character with regex (including an exception handler)
	static bool match_character(const char ch, const std::string &expression);

	/*

	Character test functions
	The following functions test to see whether a character is of a particular type to help evaluate the type of the whole token.
	Tokens all have a type and value, and are a member of one of the following classes:
		float	-	a floating point decimal number
		int		-	an integer
		string	-	any string of characters that is not an identifier or a keyword
			must be enclosed in double quotes for it to be read as a string
		bool	-	a boolean type
			values can be True or False
		ident	-	identifier
			variable names, function names, etc.
		kwd		-	keyword
			let, alloc, if, etc.
		punc	-	punctuation
			. , : ; () {} []
		op		-	mathematical, logical, and other important operators
			+ * - / & | ^ % < > = ! @ ? $
		escape	-	escaped characters
			\n, \t, etc.

	*/

	static bool is_whitespace(const char ch);	// tests if a character is \n, \t, or a space

	bool is_newline(char ch) const;	// tests for a newline character
	static bool is_not_newline(const char ch);
	static bool is_digit(const char ch);		// tests whether a character is a digit; used for the first digit in a number
	static bool is_letter(const char ch);
	static bool is_number(const char ch);		// includes a decimal point; for reading a number, not just a digit

	static bool is_id_start(const char ch);	// determine starting character of an identifier (cannot start with a number)
	static bool is_id(const char ch);			// determine if the current character is a valid id character (can contain numbers within)

	static bool is_punc(const char ch);
	static bool is_op_char(const char ch);

	static bool is_boolean(const std::string &candidate);

	static bool is_keyword(const std::string &candidate);	// test whether the string is a keyword (such as alloc or let) or an identifier (such as a variable name)

	std::string read_while(bool(*predicate)(const char));
	std::string read_operator();

	void read_lexeme();

	std::string read_string();
	std::string read_char();
	std::string read_ident();	// read the full identifier

public:
    static const std::unordered_map<std::string, exp_operator> op_strings;
	static bool is_valid_operator(const std::string &candidate);
    
    bool eof() const;		// check to see if we are at the end of the file
	bool exit_flag_is_set() const;	// check to see the status of the exit flag

	std::ostream& write(std::ostream& os) const;	// allows a lexeme to be written to an ostream

	// read the next lexeme
	lexeme read_next();

	// add a file to be lexed
	void add_file(std::istream &input);

	Lexer(std::istream& input);
	Lexer();
	~Lexer();
};

// overload the << operator
std::ostream& operator<<(std::ostream& os, const Lexer& lexer);


class LexerException : public std::exception {
	std::string message_;
	int position_;
	char ch_;
public:
	explicit LexerException(const std::string& err_message, const int err_position, const char& ch);
	virtual const char* what() const noexcept;
	char get_char();
	int get_pos();
};
