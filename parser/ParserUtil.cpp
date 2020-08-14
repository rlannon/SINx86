/*

SIN Toolchain
ParserUtil.cpp
Copyright 2019 Riley Lannon

Contains the implementations of various utility functions for the parser.

*/

#include "Parser.h"

const std::unordered_map<std::string, exp_operator> Parser::op_strings({
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
	{"::", SCOPE_RESOLUTION}
});

const std::unordered_map<exp_operator, size_t> Parser::op_precedence({
	{RIGHT_ARROW, 1},	// there is also a move assignment operator
	{LEFT_ARROW, 1},
	{PLUS_EQUAL, 1},
	{MINUS_EQUAL, 1},
	{MULT_EQUAL, 1},
	{DIV_EQUAL, 1},
	{MOD_EQUAL, 1},
	{AND_EQUAL, 1},
	{OR_EQUAL, 1},
	{XOR_EQUAL, 1},
	{OR, 2},
	{AND, 2},
	{XOR, 2},
	{BIT_AND, 3},
	{BIT_OR, 3},
	{BIT_XOR, 3},
	{LESS, 7},
	{GREATER, 7},
	{LESS_OR_EQUAL, 7},
	{GREATER_OR_EQUAL, 7},
	{EQUAL, 7},	// not the direct assignment operator -- all assignment operators have a precedence of 1
	{NOT_EQUAL, 7},
	{LEFT_SHIFT, 8},
	{RIGHT_SHIFT, 8},
	{PLUS, 10},
	{MINUS, 10},
	{MULT, 15},
	{DIV, 15},
	{MODULO, 15},
	{TYPECAST, 20},
	{NOT, 20},	// 'not' is a unary operator, so it has high priority
	{BIT_NOT, 20},
	{UNARY_PLUS, 20},
	{UNARY_MINUS, 20},
	{ADDRESS, 20},
	{DEREFERENCE, 20},
	{ATTRIBUTE_SELECTION, 23},
	{CONTROL_TRANSFER, 25},
	{DOT, 25},
	{INDEX, 25},
	{SCOPE_RESOLUTION, 30}
});

const exp_operator Parser::translate_operator(std::string op_string) {
	// try and find the operator
	std::unordered_map<std::string, exp_operator>::const_iterator it = Parser::op_strings.find(op_string);
	if (it == Parser::op_strings.end()) {
		return NO_OP;
	}
	else {
		return it->second;
	}
}

const size_t Parser::get_precedence(std::string symbol, size_t line) {
	// get the precedence of an operator based on the string

	size_t precedence = 0;
	return Parser::get_precedence(Parser::translate_operator(symbol), line);
}

const size_t Parser::get_precedence(exp_operator op, size_t line) {
	size_t precedence = 0;
	
	std::unordered_map<exp_operator, size_t>::const_iterator it = Parser::op_precedence.find(op);
	if (it == Parser::op_precedence.end()) {
		throw ParserException("Invalid operator", 0, line);
	}
	else {
		precedence = it->second;
	}

	return precedence;
}

// Utility functions for traversing the token list

bool Parser::is_at_end() {
	// Determines whether we have run out of tokens. Returns true if we have, false if not.
	if (this->position >= this->num_tokens - 2 || this->num_tokens == 0) {	// the last element is list.size() - 1;  if we are at list.size(), we have gone over
		return true;
	}
	else {
		return false;
	}
}

lexeme Parser::peek() {
	// peek to the next position
	if (position + 1 < this->tokens.size()) {
		return this->tokens[this->position + 1];
	}
	else {
		throw ParserException("No more lexemes to parse!", 1, this->tokens[this->position].line_number);
	}
}

lexeme Parser::next() {
	// Increments the position and returns the token there, provided we haven't hit the end

	// increment the position
	this->position += 1;

	// if we haven't hit the end, return the next token
	if (position < this->tokens.size()) {
		return this->tokens[this->position];
	}
	// if we have hit the end
	else {
		throw ParserException("No more lexemes to parse!", 1, this->tokens[this->position - 1].line_number);
	}
}

lexeme Parser::current_token() {
	return this->tokens[this->position];
}

lexeme Parser::previous() {
	return this->tokens[this->position - 1];
}

lexeme Parser::back() {
	this->position -= 1;
	return this->tokens[this->position];
}

void Parser::skipPunc(char punc) {
	// Skip a punctuation mark

	if (this->current_token().type == PUNCTUATION) {
		if (this->current_token().value[0] == punc) {
			this->position += 1;
			return;
		}
		else {
			return;
		}
	}
	else {
		return;
	}
}

bool Parser::is_type(std::string lex_value)
{
	// Determines whether a given string is a type name

	// todo: is there a better way to do this? using a map might not be worth it because there are so few elements

	std::vector<std::string> types = {
		"int",
		"bool",
		"string",
		"char",
		"float",
		"raw",
		"ptr",
		"array",
		"struct",
		"void"
	};

	// iterate through our list of type names
	size_t i = 0;
	bool found = false;
	auto it = types.begin();
	while (it != types.end() && !found) {
		if (lex_value == *it) {
			found = true;
		}
		else {
			it++;
		}
	}

	return found;
}

std::string Parser::get_closing_grouping_symbol(std::string beginning_symbol)
{
	/*

	Returns the appropriate closing symbol for some opening grouping symbol 'beginning_symbol'; e.g., if beginning_symbol is '(', the function will return ')'

	Operates with strings because that's what lexemes use

	*/

	if (beginning_symbol == "(") {
		return ")";
	}
	else if (beginning_symbol == "[") {
		return "]";
	}
	else if (beginning_symbol == "{") {		// while curly braces are not considered grouping symbols by these functions, we will include it here
		return "}";
	}
	else if (beginning_symbol == "<") {
		return ">";
	}
	else {
		throw ParserException("Invalid grouping symbol in expression!", compiler_errors::INVALID_TOKEN, 0);
		return "";
	}
}

bool Parser::is_opening_grouping_symbol(std::string to_test)
{
	/*

	Checks to see whether a given string is a opening parenthesis or a bracket.
	Curly braces not included here because they are not considered grouping symbols like ( and [ -- they serve a different purpose

	*/
	return (to_test == "(" || to_test == "[");
}

const bool Parser::has_return(StatementBlock to_test)
{
	/*
	
	Checks to see whether a statement block 'to_test' has a return statement in it.
	This is useful for checking to ensure all of our functions have return statements in them.
	This can be modified in the future to test whether all control paths return in a function, which is required. For now, keep it simple.
	
	*/

	// our base case is that the statement block has a return statement
	if (to_test.has_return) {
		return true;
	}
	// if it doesn't, check to see whether we have a scope block in the last statement
	else {
		// if we have no statements in the scope block, we obviously have no return statement
		if (to_test.statements_list.size() == 0) {
			return false;
		}
		else {
			/*
			
			The last statement in the function _must_ contain a return. For example:
				def void myFunc(alloc int myVar) {
					if (myVar = 0) {
						return void;
					}
					else {
						@print("myVar is not 0");
					}

					@print("unreachable");
				}
				
				This will generate the error that the last print is unreachable, but it should also generate the error that the function does not return.
				This is because the last statement has no return.
			
			*/
			
			// get the last statement and check its type
			Statement* last_statement = to_test.statements_list.back().get();
			if (last_statement->get_statement_type() == IF_THEN_ELSE) {
				IfThenElse* ite = dynamic_cast<IfThenElse*>(last_statement);

				bool returns = general_utilities::ite_returns(ite);

				// if both branches return a value, we are golden
				if (!returns) {
					throw NoReturnException(last_statement->get_line_number());
				}
				
				return returns;
			}
			else if (last_statement->get_statement_type() == WHILE_LOOP) {
				WhileLoop* while_loop = dynamic_cast<WhileLoop*>(last_statement);

				// while loops are a little simpler, we can simply pass in the branch for the while loop
				return general_utilities::returns(while_loop->get_branch());
			}
			else {
				return false;
			}
		}
	}
}

DataType Parser::get_type(std::string grouping_symbol)
{
    /*

    Parses type information and stores it in a DataType object

	Parser token should start with current_lex as the first token in the type data

    */

	// check our qualities, if any
	symbol_qualities qualities = this->get_prefix_qualities(grouping_symbol);

	// todo: should we set the 'dynamic' quality if we have a string?

	// get the current lexeme
	lexeme current_lex = this->current_token();

	Type new_var_type;
	DataType new_var_subtype;
	std::shared_ptr<Expression> array_length_exp = nullptr;
	std::string struct_name = "";

	if (current_lex.value == "ptr" || current_lex.value == "ref") {
		// set the type
		new_var_type = current_lex.value == "ptr" ? PTR : REFERENCE;

		// 'ptr' must be followed by '<'
		if (this->peek().value == "<") {
			this->next();
			
			new_var_subtype = this->parse_subtype("<");
		}
		// if it's not, we have a syntax error
		else {
			throw ParserException(
				"Proper syntax is 'alloc ptr< T >' or 'alloc ref< T >'",
				compiler_errors::INVALID_TYPE_SYNTAX,
				current_lex.line_number
			);
		}
	}
	// otherwise, if it's an array,
	else if (current_lex.value == "array") {
		new_var_type = ARRAY;
		// check to make sure we have the size and type in angle brackets
		if (this->peek().value == "<") {
			this->next();	// eat the angle bracket

			// if the next value is a keyword, we can leave array_length_exp as a nullptr
			if (this->peek().type == KEYWORD_LEX) {
				new_var_subtype = this->parse_subtype("<");
			} else {
				// parse an expression to obtain the array length; the _current lexeme_ should be the first lexeme of the expression		
				this->next();
				array_length_exp = this->parse_expression();
				
				// the array length will be evaluated by the compiler; continue parsing

				// a comma should follow the size
				if (this->peek().value == ",") {
					this->next();
						
					// parse a full type
					new_var_subtype = this->parse_subtype("<");
				}
				else {
					throw ParserException("The size of an array must be followed by the type", 0, current_lex.line_number);
				}
			}
		}
		else {
			throw ParserException("You must specify the size and type of an array (in that order)", 0, current_lex.line_number);
		}
	}
	else if (current_lex.value == "tuple") {
		// tuples contain an arbitrarily long list of types
	}
	// otherwise, if it does not have a contained type, it is either a different type (keyword) or struct (identifier)
	else if (current_lex.type == KEYWORD_LEX || current_lex.type == IDENTIFIER_LEX) {
		// if we have an int, but we haven't pushed back signed/unsigned, default to signed
		if (current_lex.value == "int") {
			// if our symbol doesn't have signed or unsigned, set, it must be signed by default
			if (!qualities.is_signed() && !qualities.is_unsigned()) {
				qualities.add_quality(SIGNED);
			}
		}

		// store the type name in our Type object
		new_var_type = type_deduction::get_type_from_string(current_lex.value);

		// if we have a struct, make a note of the name
		if (new_var_type == STRUCT) {
			// if we didn't have a valid type name, but it was a keyword, then throw an exception -- the keyword used was not a valid type identifier
			if (current_lex.type == KEYWORD_LEX) {
				throw ParserException(("Invalid type specifier '" + current_lex.value + "'"), 0, current_lex.line_number);
			}

			struct_name = current_lex.value;
		}
	}
	else {
		throw ParserException(
			("'" + current_lex.value + "' is not a valid type name"),
			compiler_errors::MISSING_IDENTIFIER_ERROR,
			current_lex.line_number
		);
	}

	// create the symbol type data
	DataType symbol_type_data(new_var_type, new_var_subtype, qualities, array_length_exp, struct_name);
	return symbol_type_data;
}

DataType Parser::parse_subtype(std::string grouping_symbol) {
	/*

	parse_subtype
	Parses a subtype, postfix qualities and all

	For this function, the current token of the parser must be _on_ the grouping symbol
	This function will also end with the current token of the parser _on_ the closing symbol

	@param	grouping_symbol	The grouping symbol for the subtype -- should always be "<" in practice
	@return	The parsed DataType object

	*/

	this->next();	// eat the opening grouping symbol

	// first, parse the type (which will handle prefixed qualities)
	DataType new_var_subtype;
	new_var_subtype = this->get_type(grouping_symbol);
	
	// since subtypes have no 'default values', parse postfixed qualities, if there are any
	if (this->peek().value == "&") {
		this->next();	// eat the ampersand

		symbol_qualities postfixed_qualities = this->get_postfix_qualities(grouping_symbol);
		new_var_subtype.add_qualities(postfixed_qualities);
	}

	// ensure an angle bracket follows our postfixed qualities and eat it
	if (this->peek().value == get_closing_grouping_symbol(grouping_symbol)) {
		this->next();
	} else {
		throw ParserException("Unclosed grouping symbol found", 0, this->current_token().line_number);
	}

	return new_var_subtype;
}

symbol_qualities Parser::get_prefix_qualities(std::string grouping_symbol) {
	/*

	get_prefix_qualities
	Gets the symbol qualities placed before the symbol

	The current lexeme should be the start of the qualities

	@param	grouping_symbol	We may have
	@return	A SymbolQualities object

	*/

	symbol_qualities qualities;

	// loop until we don't have a quality token, at which point we should return the qualities object
	lexeme current = this->current_token();
	while (current.type == KEYWORD_LEX && !is_type(current.value)) {
		// get the current quality and add it to our qualities object
		try {
			qualities.add_quality(get_quality(current));
		} catch (std::string &offending_quality) {
			// catch the exception thrown by 'add quality' and throw a new one with a line number
			throw QualityConflictException(offending_quality, current.line_number);
		}

		// advance the token position
		current = this->next();
	}

	return qualities;
}

symbol_qualities Parser::get_postfix_qualities(std::string grouping_symbol)
{
	/*

	Symbol qualities can be given after an allocation -- as such, the following statements are valid:
		alloc const unsigned int x: 10;
		alloc int x: 10 &const unsigned;

	This function begins on the first token of the postfix quality -- that is to say, "current_token" should be the ampersand.

	*/

	// get the closing grouping symbol, if applicable
	std::string closing_symbol;
	if (is_opening_grouping_symbol(grouping_symbol) || grouping_symbol == "<") {	// todo: include < in function?
		closing_symbol = get_closing_grouping_symbol(grouping_symbol);
	}
	else {
		closing_symbol = "";
	}

	symbol_qualities qualities;	// create our qualities vector; initialize to an empty vector

	// continue parsing our SymbolQualities until we hit a semicolon, at which point we will trigger the 'done' flag
	bool done = false;
	while (this->peek().type == KEYWORD_LEX) {
		lexeme quality_token = this->next();	// get the token for the quality
		SymbolQuality quality = this->get_quality(quality_token);	// use our 'get_quality' function to get the SymbolQuality based on the token

		// try adding our qualities, throw an error if there is a conflict
		try {
			qualities.add_quality(quality);
		} catch (CompilerException &e) {
			throw QualityConflictException(quality_token.value, quality_token.line_number);
		}
	}

	return qualities;
}

SymbolQuality Parser::get_quality(lexeme quality_token)
{
	// Given a lexeme containing a quality, returns the appropriate member from SymbolQuality

	SymbolQuality to_return = NO_QUALITY;

	// ensure the token is a kwd
	if (quality_token.type == KEYWORD_LEX) {
		// Use the unordered_map to find the quality
		auto it = symbol_qualities::quality_strings.find(quality_token.value);
		
		if (it == symbol_qualities::quality_strings.end()) {
			throw CompilerException("Invalid qualifier", compiler_errors::EXPECTED_SYMBOL_QUALITY, quality_token.line_number);
		}
		else {
			to_return = it->second;
		}
	}
	else {
		throw CompilerException("Invalid qualifier", compiler_errors::EXPECTED_SYMBOL_QUALITY, quality_token.line_number);
	}

	return to_return;
}

calling_convention Parser::get_calling_convention(symbol_qualities sq, unsigned int line) {
	// Given a 'symbolqualities' object, gets the calling convention

	calling_convention c = SINCALL;

	// ensure we only specified one convention (although sincall is the default convention, this quality shows whether the user specified it)
	if (
		(sq.is_c64() && sq.is_sincall()) ||
		(sq.is_sincall() && sq.is_windows())
	) {
		throw CompilerException("Only one calling convention may be specified", compiler_errors::ILLEGAL_QUALITY_ERROR, line);
	} else {
		// it defaults to sincall, so we only need to test against the others
		if (sq.is_c64() && sq.is_windows())
			c = calling_convention::WIN_64;
		else if (sq.is_c64())
			c = calling_convention::SYSTEM_V;
		else if (sq.is_windows())	// if 'windows' is used by itself, generate an error -- it must occur with 'c64'
			throw CompilerException(
				"Use of the 'windows' keyword must also use the 'c64' keyword if it is to specify a calling convention",
				compiler_errors::ILLEGAL_QUALITY_ERROR,
				line);
	}

	return c;
}

const bool Parser::is_valid_operator(lexeme l) {
	// Checks whether the lexeme is a valid operator for maybe_binary
	return (
		l.type == OPERATOR ||
		l.value == "[" ||
		l.value == "as" ||
		l.value == "and" ||
		l.value == "or" || 
		l.value == "xor"
	);
}

const exp_operator Parser::get_unary_operator(std::string s) {
	/*

	Gets an exp_operator from a string; if it is not a valid unary operator, returns NO_OP

	*/

	exp_operator op;

	if (s == "+") {
		op = UNARY_PLUS;
	}
	else if (s == "-") {
		op = UNARY_MINUS;
	}
	else if (s == "$") {
		op = ADDRESS;
	}
	else if (s == "*") {
		op = DEREFERENCE;
	}
	else if (s == "not") {
		op = NOT;
	}
	else if (s == "~") {
		op = BIT_NOT;
	}
	else {
		op = NO_OP;
	}
	
	return op;
}
