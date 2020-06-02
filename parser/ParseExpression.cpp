/*

SIN Toolchain
ParseExpression.cpp
Copyright 2019 Riley Lannon

Contains the implementations of the functions to parse expressions, including:
	- std::shared_ptr<Expression> parse_expression(size_t prec=0, std::string grouping_symbol = "(", bool not_binary = false);
	- std::shared_ptr<Expression> create_dereference_object();
	- LValue getDereferencedLValue(Dereferenced to_eval);
	- std::shared_ptr<Expression> maybe_binary(std::shared_ptr<Expression> left, size_t my_prec, std::string grouping_symbol = "(");

*/

#include "Parser.h"

std::shared_ptr<Expression> Parser::parse_expression(size_t prec, std::string grouping_symbol, bool not_binary, bool omit_equals) {
	lexeme current_lex = this->current_token();

	// Create a pointer to our first value
	std::shared_ptr<Expression> left;
	bool is_const = false;

	// first, check to see if we have the 'constexpr' keyword
	if (current_lex.value == "constexpr") {
		is_const = true;
		current_lex = this->next();	// update the current lexeme
	}

	// Check if our expression begins with a grouping symbol; if so, only return what is inside the symbols
	// note that curly braces are NOT included here; they are parsed separately as they are not considered grouping symbols in the same way as parentheses and brackets are
	if (is_opening_grouping_symbol(current_lex.value)) {
		grouping_symbol = current_lex.value;
		this->next();
		left = this->parse_expression(0, grouping_symbol);
		this->next();

		/*

		if we are getting the expression within an indexed expression, we don't want to parse out a binary (otherwise it might parse:
			let myArray[3] = 0;
		as having the expression 3 = 0, which is not correct

		*/
		if (this->current_token().value == "]" && not_binary) {
			return left;
		}

		// Otherwise, carry on parsing

		// check to see if we have a postfixed '&constexpr'
		if (this->peek().value == "&") {
			this->next();

			// if we have "constexpr" next, then parse it; else, move back
			if (this->peek().value == "constexpr") {
				this->next();
				is_const = true;
			} else {
				this->back();
			}
		}

		// now, if we had prefixed _or_ postfixed 'constexpr', set the const value
		if (is_const) left->set_const();

		// if our next character is a semicolon or closing paren, then we should just return the expression we just parsed
		if (this->peek().value == ";" || this->peek().value == get_closing_grouping_symbol(grouping_symbol) || this->peek().value == "{") {
			return left;
		}
		// if our next character is an op_char, returning the expression would skip it, so we need to parse a binary using the expression in parens as our left expression
		else if (this->peek().type == OPERATOR) {
			return this->maybe_binary(left, prec, grouping_symbol);
		}
	}
	// list expressions (array literals) have to be handled slightly differently than other expressions
	else if (current_lex.value == "{") {
		this->next();
		std::vector<std::shared_ptr<Expression>> list_members = {};

		// as long as the next token is a comma, we have elements to parse
		while (this->peek().value != "}" && this->peek().value != ";") {
			list_members.push_back(this->parse_expression(prec, "{"));
			this->next();	// skip the last character of the expression
		}

		// once we escape the loop, we must find a closing curly brace
		if (this->peek().value == ";") {
			left = std::make_shared<ListExpression>(list_members);
		}
		else {
			throw InvalidTokenException(this->peek().value, this->peek().line_number);
		}
	}
	// if expressions are separated by commas, continue parsing the next one
	else if (current_lex.value == ",") {
		this->next();
		return this->parse_expression(prec, grouping_symbol, not_binary);
	}
	// if it is not an expression within a grouping symbol, it is parsed below
	else if (is_literal(current_lex.type)) {
		left = std::make_shared<Literal>(
			type_deduction::get_type_from_lexeme(current_lex.type),
			current_lex.value
		);
	}
	else if (current_lex.type == IDENTIFIER) {
		// check to see if we have the identifier alone, or whether we have an index
		if (this->peek().value == "[") {
			this->next();
			left = std::make_shared<Indexed>(current_lex.value, "var", this->parse_expression(0, "[", true));
		}
		else {
			left = std::make_shared<LValue>(current_lex.value);
		}
	}
	// if we have a keyword to begin an expression, parse it (could be a sizeof expression)
	else if (current_lex.type == KEYWORD) {
		if (current_lex.value == "sizeof") {
			// expression must be enclosed in angle brackets
			if (this->peek().value == "<") {
				grouping_symbol = "<";
				this->next();
				
				// use our type parsing function to parse the sizeof< T > type
				DataType to_check = this->parse_subtype("<");
				left = std::make_shared<SizeOf>(to_check);

				// sizeof expressions are compile-time constants
				is_const = true;
			}
			else {
				throw ParserException("Syntax error; expected '<'", 0, current_lex.line_number);
			}
		}
		else {
			throw UnexpectedKeywordError(current_lex.value, current_lex.line_number);
		}
	}
	// if we have an op_char to begin an expression, parse it (could be a pointer or a function call)
	else if (current_lex.type == OPERATOR) {
		// if we have a function call
		if (current_lex.value == "@") {
			current_lex = this->next();

			if (current_lex.type == IDENTIFIER) {
				// Same code as is in statement
				std::vector<std::shared_ptr<Expression>> args;

				// make sure we have parens -- if not, throw an exception
				if (this->peek().value != "(") {
					throw CallError(current_lex.line_number);
				} else {
					this->next();
					this->next();
					while (this->current_token().value != get_closing_grouping_symbol(grouping_symbol)) {
						args.push_back(this->parse_expression());
						this->next();
					}
				}

				// assemble the value returning call so we can pass into maybe_binary
				left = std::make_shared<ValueReturningFunctionCall>(std::make_shared<LValue>(current_lex.value, "func"), args);
			}
			// the "@" character must be followed by an identifier
			else {
				throw MissingIdentifierError(current_lex.line_number);
			}
		}
		// check to see if we have the address-of operator
		else if (current_lex.value == "$") {
			// if we have a $ character, it HAS TO be the address-of operator
			// current lexeme is the $, so get the variable for which we need the address
			lexeme next_lexeme = this->next();
			left = this->parse_expression(get_precedence(exp_operator::ADDRESS));
			// todo: ensure address-of expressions parse correctly
		}
		// check to see if we have a pointer dereference operator
		else if (current_lex.value == "*") {
			left = this->create_dereference_object();
		}
		// check to see if we have a unary operator
		else if ((current_lex.value == "+") || (current_lex.value == "-") || (current_lex.value == "!") || (current_lex.value == "~")) {
			// get the precedence of the unary operator
			size_t precedence;
			if (current_lex.value == "+") precedence = Parser::get_precedence(UNARY_PLUS);
			else if (current_lex.value == "-") precedence = Parser::get_precedence(UNARY_MINUS);
			else if (current_lex.value == "!") precedence = Parser::get_precedence(NOT);
			else precedence = Parser::get_precedence(BIT_NOT);

			// advance the token pointer and parse the expression
			this->next();
			std::shared_ptr<Expression> operand = this->parse_expression(precedence);	// parse an expression at the precedence level of our unary operator

			// todo: simplify this a little
			// now that we have the operand, create the expression
			if (current_lex.value == "+") {
				left = std::make_shared<Unary>(operand, PLUS);
			}
			else if (current_lex.value == "-") {
				left = std::make_shared<Unary>(operand, MINUS);
			}
			else if (current_lex.value == "!") {
				left = std::make_shared<Unary>(operand, NOT);
			}
			else {
				left = std::make_shared<Unary>(operand, BIT_NOT);
			}
		}
	}

	// peek ahead at the next symbol; we may have a postfixed constexpr quality
	if (this->peek().value == "&") {
		// eat the ampersand
		this->next();
		lexeme quality = this->next();
		if (quality.value == "constexpr") {
			is_const = true;
		} else {
			this->back();
			this->back();
			// throw IllegalQualityException(quality.value, quality.line_number);
		}

		// do not advance token; we use 'peek' in maybe_binary
	}

	// if is_const is set, then set 'left' to be a constexpr
	if (is_const || left->get_expression_type() == LITERAL) left->set_const();

	// Use the maybe_binary function to determine whether we need to return a binary expression or a simple expression

	// always start it at 0; the first time it is called, it will be 0, as nothing will have been passed to parse_expression, but will be updated to the appropriate precedence level each time after. This results in a binary tree that shows the proper order of operations
	if (not_binary) {
		return left;
	}
	else {
		if (this->peek().value == "=" && omit_equals) {
			return left;
		}
		else {
			return this->maybe_binary(left, prec, grouping_symbol, omit_equals);
		}
	}
}


// Create a Dereferenced object when we dereference a pointer
std::shared_ptr<Expression> Parser::create_dereference_object() {
	/*
	
	create_dereference_object
	Parses out a 'Dereferenced' expression

	Although the asterisk can be either the multiplication operator or the dereference operator, it is impossible for the parser to get them confused because they appear in completely different contexts.
	This function creates a dereferenced expression by parsing the expression that is contained within it
	
	*/

	this->next();
	Dereferenced deref(this->parse_expression());
	return std::make_shared<Dereferenced>(deref);
}

std::shared_ptr<Expression> Parser::maybe_binary(std::shared_ptr<Expression> left, size_t my_prec, std::string grouping_symbol, bool omit_equals) {
	/*

	maybe_binary
	Determines whether an expression is part of a binary expression

	Determines whether the expression 'left', with a precedence of 'my_prec', is a part of a larger binary expression. If so, creates that binary expression according to operator precedence levels.
	For example, if we pass in the expression:
		3 + 4 * 5 - 6;
	The expression 3 will be passed into this function with a my_prec value of 0. Since + follows, we know it must be a binary expression. However, the right operand may be the left operand of a binary operation at a higher precedence level, so we must call this function recursively to create the right operand. * has a higher precedence than +, so our right-hand operator will be a binary. - has a lower precedence than *, so it is ignored when crafting the right-hand side.
	Once the expression 3 + (4 * 5) is crafted, we call maybe_binary again at the *old* precedence level on the new binary expression to see if it is a part of a larger binary expression. It is, because it is followed by - 6. So, we create the expression (3 + (4 * 5)) - 6.
	This algorithm ensures we are using the correct order of operations.
	
	@param	left	The expression that may be the left operand of a binary expression
	@param	my_prec	The current precedence level
	@param	grouping_symbol	We may be inside a grouped evaluation; if so, this tracks the grouping symbol used

	@return	An expression; may be a binary expression, may not be

	*/

	lexeme next = this->peek();

	// if the next character is a semicolon, another end paren, or a comma, return
	if (next.value == ";" || next.value == get_closing_grouping_symbol(grouping_symbol) || next.value == "," || (next.value == "=" && omit_equals)) {
		return left;
	}
	// Otherwise, if we have an op_char or the 'and', 'or', or 'xor'
	else if (next.type == OPERATOR || next.value == "and" || next.value == "or" || next.value == "xor") {
		// if the operator is '&', it could be used for bitwise-and OR for postfixed symbol qualities; if the token following is a keyword, it cannot be bitwise-and
		if (next.value == "&") {
			this->next();	// advance the iterator so we can see what comes after the ampersand
			lexeme operand = this->peek();
			
			// if the operand is a keyword, the & must not be intended to be the bitwise-and operator
			if (operand.type == KEYWORD) {
				this->back();	// move the iterator back
				return left;	// return our left argument
			}
			else {
				this->back();	// move the iterator back where it was
			}
		}
		
		// if we have the 'as' keyword, we will immediately parse out a typecast expression with the left operand
		if (next.value == "as") {
			this->next();
			this->next();	// skip ahead to the first lexeme of the data type
			DataType new_type = this->get_type();
			auto cast_exp = std::make_shared<Cast>(left, new_type);
			return cast_exp;
		}
		// otherwise, parse the binary expression as usual
		else {
			// get the next op_char's data
			size_t his_prec = get_precedence(next.value, next.line_number);

			// If the next operator is of a higher precedence than ours, we may need to parse a second binary expression first
			if (his_prec > my_prec) {
				this->next();	// go to the next character in our stream (the op_char)
				this->next();	// go to the character after the op char

				// Parse out the next expression
				std::shared_ptr<Expression> right = this->maybe_binary(this->parse_expression(his_prec, grouping_symbol), his_prec, grouping_symbol, omit_equals);	// make sure his_prec gets passed into parse_expression so that it is actually passed into maybe_binary

				// Create the binary expression
				std::shared_ptr<Binary> binary = std::make_shared<Binary>(left, right, translate_operator(next.value));	// "next" still contains the op_char; we haven't updated it yet

				// if the left and right sides are constants, the whole expression is a constant
				if (left->is_const() && right->is_const())
					binary->set_const();

				// call maybe_binary again at the old prec level in case this expression is followed by one of a higher precedence
				return this->maybe_binary(binary, my_prec, grouping_symbol, omit_equals);
			}
			else {
				return left;
			}
		}
	}
	// There shouldn't be anything besides a semicolon, closing paren, or an op_char immediately following "left"
	else {
		throw InvalidTokenException(next.value, next.line_number);
	}
}
