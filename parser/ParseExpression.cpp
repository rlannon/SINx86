/*

SIN Toolchain
ParseExpression.cpp
Copyright 2019 Riley Lannon

Contains the implementations of the functions to parse expressions.

*/

#include "Parser.h"

std::unique_ptr<Expression> Parser::parse_expression(
	const size_t prec,
	std::string grouping_symbol,
	bool not_binary,
	const bool omit_equals
) {
	/*

	parse_expression
	Parses an expression

	*/

	lexeme current_lex = this->current_token();

	// Create a pointer to our first value
	std::unique_ptr<Expression> left = nullptr;
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
        std::unique_ptr<Expression> temp = nullptr;
        
        // we might have an empty list
        if (this->peek().value == get_closing_grouping_symbol(grouping_symbol)) {
            temp = std::make_unique<ListExpression>();
        }
        else {
    		this->next();
		    temp = this->parse_expression(0, grouping_symbol);	// utilize a 'temp' variable in case we have a list
        }

		/*

		if we are getting the expression within an indexed expression, we don't want to parse out a binary (otherwise it might parse:
			let myArray[3] = 0;
		as having the expression 3 = 0, which is not correct

		*/
		if (this->peek().value == "]" && not_binary) {
			this->next();
			return temp;
		}
		else if (this->peek().value == get_closing_grouping_symbol(grouping_symbol)) {
			this->next();
		}

		// Otherwise, carry on parsing

		// check to see if we have a postfixed '&constexpr'
		if (this->peek().value == "&") {
			this->next();

			// if we have "constexpr" next, then parse it; else, move back
			// todo: quality overrides
			if (this->peek().value == "constexpr") {
				this->next();
				is_const = true;
			} else {
				this->back();
			}
		}

		// now, if we had prefixed _or_ postfixed 'constexpr', set the const value
		if (is_const) temp->set_const();

		// if our next character is a closing paren, then we should just return the expression we just parsed
		if (this->peek().value == get_closing_grouping_symbol(grouping_symbol)) {
			return temp;
		}
        else if (this->peek().value == ";") {
            return temp;
        }
		// if our next character is an op_char, returning the expression would skip it, so we need to parse a binary using the expression in parens as our left expression
		else if (is_valid_operator(this->peek())) {
            if (not_binary)
                return temp;
            else
    			return this->maybe_binary(std::move(temp), prec, grouping_symbol);
		}
		// if we had a comma, we need to parse a list
		else if (this->peek().value == ",") {
			// ensure we have a valid grouping symbol for our list and set the expression's primary type accordingly
			const std::string& list_grouping_symbol = current_lex.value;
			Type list_type;
			if (list_grouping_symbol == "(") {
				list_type = TUPLE;
			}
			else if (list_grouping_symbol == "{") {
				list_type = ARRAY;
			}
			else {
				throw ParserException(
					"Illegal list grouping symbol",
					compiler_errors::INVALID_TYPE_SYNTAX,
					this->current_token().line_number
				);
			}

			// set this to false if any element is *not* const
			is_const = true;

			// create a copy of 'left' because 'left' will need to hold the list expression -- and contain the value currently in 'left'
			std::vector<std::unique_ptr<Expression>> list_members;
			list_members.push_back(std::move(temp));
			left = nullptr;

			// as long as the next character is not a comma, we have more lexemes to parse
			lexeme peeked = this->peek();

			while (peeked.value != get_closing_grouping_symbol(list_grouping_symbol)) {
				this->next();	// skip the last character of the expression (on comma)
				try {
					auto elem = this->parse_expression(prec, list_grouping_symbol);
					if (!elem->is_const())
						is_const = false;
					
					list_members.push_back(std::move(elem));
				}
				catch (std::exception &e) {
					throw ParserException(
						"Unexpected token while parsing list expression",
						compiler_errors::INVALID_TOKEN,
						this->current_token().line_number
					);
				}
				peeked = this->peek();
			}
			this->next();

			// once we escape the loop, we must find a closing grouping symbol
			if (this->current_token().value != get_closing_grouping_symbol(list_grouping_symbol)) {
				throw UnclosedGroupingSymbolError(this->current_token().line_number);
			}

			left = std::make_unique<ListExpression>(list_members, list_type);
			not_binary = true;	// list literals are not allowed to be a part of binary expressions because dynamically resizable arrays are not first class types
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
		left = std::make_unique<Literal>(
			type_deduction::get_type_from_lexeme(current_lex.type),
			current_lex.value
		);
	}
	else if (current_lex.type == IDENTIFIER_LEX) {
		// make an LValue expression
		left = std::make_unique<Identifier>(current_lex.value);
	}
	// if we have a keyword to begin an expression (could be 'not' or an attribute selection like int:size)
	else if (current_lex.type == KEYWORD_LEX) {
		if (current_lex.value == "not") {
			// the logical not operator
			this->next();
			auto negated = this->parse_expression(get_precedence(NOT, current_lex.line_number));
			left = std::make_unique<Unary>(std::move(negated), NOT);
		}
		else if (AttributeSelection::is_attribute(current_lex.value)) {
			// if we have an attribute, parse out a keyword expression
			left = std::make_unique<KeywordExpression>(current_lex.value);
		}
		else {
			try {
				auto t = this->get_type(grouping_symbol);
				left = std::make_unique<KeywordExpression>(t);
			} catch (ParserException& e) {
				throw UnexpectedKeywordError(current_lex.value, current_lex.line_number);
			}
		}
	}
	// if we have an op_char to begin an expression, parse it (could be a pointer or a function call)
	else if (current_lex.type == OPERATOR) {
		// if we have a function call
		if (current_lex.value == "@") {
			current_lex = this->next();
            auto func_name = this->parse_expression(get_precedence(exp_operator::CONTROL_TRANSFER));
            if (func_name->get_expression_type() == PROC_EXP) {
                auto proc_exp = static_cast<Procedure*>(func_name.get());
                left = std::make_unique<CallExpression>(proc_exp);
            }
            else {
                // todo: valid call expressions without proc objects
                throw ParserException("Expected procedure expression", compiler_errors::UNSUPPORTED_FEATURE, current_lex.line_number);
            }
    	}
		// if it's not a function, it must be a unary expression
		else {
			exp_operator unary_op = Parser::get_unary_operator(current_lex.value);
			if (unary_op == NO_OP) {
				// throw exception -- invalid unary op
				throw ParserException(
					"'" + current_lex.value + "' is not a valid unary operator",
					compiler_errors::OPERATOR_TYPE_ERROR,
					current_lex.line_number
				);
			}
			else {
				// get the precedence of the unary operator
				size_t precedence = Parser::get_precedence(unary_op);

				// advance the token pointer and parse the expression
				this->next();
				auto operand = this->parse_expression(precedence);	// parse an expression at the precedence level of our unary operator
				left = std::make_unique<Unary>(std::move(operand), unary_op);
			}
		}
	}
	// for safety, we need an else case
	else {
		throw InvalidTokenException(this->peek().value, this->peek().line_number);
	}

	// peek ahead at the next symbol; we may have a postfixed quality for constexpr or a quality override
	if (this->peek().value == "&") {
		// todo: allow type quality overrides

		// eat the ampersand
		this->next();
		lexeme quality = this->peek();
		if (quality.type == KEYWORD_LEX) {
			// try getting a symbol quality
			if (quality.value == "constexpr") {
				this->next();
				is_const = true;
			}
			else {
				symbol_qualities sq;
				try {
					sq = get_postfix_qualities(grouping_symbol);
				}
				catch (std::exception &e) {
					throw ParserException(
						"Expected postfixed type qualifier", 
						compiler_errors::EXPECTED_SYMBOL_QUALITY, 
						this->current_token().line_number
					);
				}

				// if this expression has known type information at the parse stage, we can utilize this -- else, we need a typecast
				if (left->has_type_information()) {
					left->override_qualities(sq);
				}
				else {
					throw ParserException(
						"Expressions of this type may not utilize quality overrides; use a proper typecast instead",
						compiler_errors::UNEXPECTED_SYMBOL_QUALITY,
						this->current_token().line_number
					);
				}
			}
		} else {
			this->back();
		}

		// do not advance token; we use 'peek' in maybe_binary
	}

	// if is_const is set, then set 'left' to be a constexpr
	if (is_const || left->get_expression_type() == LITERAL) left->set_const();

	// Use the maybe_binary function to determine whether we need to return a binary expression or a simple expression

	// always start it at 0; the first time it is called, it will be 0, as nothing will have been passed to parse_expression, but will be updated to the appropriate precedence level each time after
	// This results in a binary tree that shows the proper order of operations
	if (not_binary) {
		return left;
	}
	else {
		// check to see if we have a valid operator
		exp_operator peek_op = this->read_operator(true);
		if (
			(is_valid_copy_assignment_operator(peek_op) || is_valid_move_assignment_operator(peek_op))
			&& omit_equals
		) {
			return left;
		}
		
		return this->maybe_binary(std::move(left), prec, grouping_symbol, omit_equals);
	}
}

std::unique_ptr<Expression> Parser::maybe_binary(
	std::unique_ptr<Expression> left,
	const size_t my_prec,
	const std::string& grouping_symbol,
	const bool omit_equals
) {
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
	if (
		next.value == ";" || 
		next.value == get_closing_grouping_symbol(grouping_symbol) || 
		next.value == "," || 
		(next.value == "=" && omit_equals)
	) {
		return left;
	}
	else if (is_valid_operator(next)) {
		// get the operator
		auto op = this->read_operator(true);

		// if the operator is LEFT_ARROW or RIGHT_ARROW, it's not a binary expression -- they're for movement only
		if (op == LEFT_ARROW || op == RIGHT_ARROW) {
			return left;
		}

		// if the operator is '&', it could be used for bitwise-and OR for postfixed symbol qualities; if the token following is a keyword, it cannot be bitwise-and
		if (op == BIT_AND) {
			this->next();	// advance the iterator so we can see what comes after the ampersand
			lexeme operand = this->peek();
			
			// if the operand is a keyword, the & must not be intended to be the bitwise-and operator
			if (operand.type == KEYWORD_LEX) {
				this->back();	// move the iterator back
				return left;	// return our left argument
			}
			else {
				this->back();	// move the iterator back where it was
			}
		}
		
		// parse the binary expression as usual
		// get the next op_char's data
		size_t his_prec = get_precedence(op, next.line_number);

		// If the next operator is of a higher precedence than ours, we may need to parse a second binary expression first
		if (his_prec > my_prec) {
			// we peeked the operator before, so now we should skip over it
			this->read_operator(false);
			this->next();

			std::unique_ptr<Expression> to_check = nullptr;

			// we might have an indexed expression here
			if (op == INDEX) {
				auto index_value = this->parse_expression(0, "[");	// the prec level should be zero because it is an isolated expression
				this->next();
				to_check = std::make_unique<Indexed>(std::move(left), std::move(index_value));
			}
            else if (op == PROC_OPERATOR) {
                // Procedures require a little special care as well
                this->back();
                auto arg_exp = this->parse_expression(0, grouping_symbol, true, omit_equals);
                
                if (arg_exp->get_expression_type() == LIST) {
                    to_check = std::make_unique<Procedure>(
						std::move(left), 
						std::unique_ptr<ListExpression>(
							static_cast<ListExpression*>(arg_exp.release())
						)
					);
                }
                else if (this->current_token().value == ")") {
                    // if there was only one argument, the parser will emit that expression alone
                    auto l = std::make_unique<ListExpression>(std::unique_ptr<Expression>{ std::move(arg_exp) }, TUPLE);
                    to_check = std::make_unique<Procedure>(std::move(left), std::move(l));
                }
                else {
                    throw ParserException(
                        "Expected argument list expression",
                        compiler_errors::INVALID_EXPRESSION_TYPE_ERROR,
                        next.line_number
                    );
                }

                return to_check;
            }
			else {
				// Parse out the next expression using maybe_binary (in case there is another operator of a higher precedence following this one)
				auto right = this->maybe_binary(
					this->parse_expression(
						his_prec, grouping_symbol, false, omit_equals
					),
					his_prec,
					grouping_symbol,
					omit_equals
				);	// make sure his_prec gets passed into parse_expression so that it is actually passed into maybe_binary

				// Create the binary expression
				auto binary = std::make_unique<Binary>(std::move(left), std::move(right), op);	// "next" still contains the op_char; we haven't updated it yet

				// if the left and right sides are constants, the whole expression is a constant
				if (binary->get_left().is_const() && binary->get_right().is_const())
					binary->set_const();
				
				// now, call maybe_binary based on the binary type (transform the statement)
				if (binary->get_operator() == ATTRIBUTE_SELECTION) {
					to_check = std::make_unique<AttributeSelection>(
						std::unique_ptr<Binary>(
							static_cast<Binary*>(binary.release())
						)
					);
				}
				else if (binary->get_operator() == TYPECAST) {
					to_check = std::make_unique<Cast>(std::move(binary));
				}
				else
				{
					to_check = std::move(binary);
				}
				

				// ensure we still have a valid expression
				if (to_check->get_expression_type() == EXPRESSION_GENERAL) {
					throw CompilerException(
						"Illegal expression",
						compiler_errors::INVALID_EXPRESSION_TYPE_ERROR,
						next.line_number
					);
				}
			}
			
			// call maybe_binary again at the old prec level in case this expression is part of a higher precedence one
			return this->maybe_binary(std::move(to_check), my_prec, grouping_symbol, omit_equals);
		}
		else {
			return left;
		}
	}
	else {
		throw InvalidTokenException(next.value, next.line_number);
	}
}

exp_operator Parser::get_compound_arithmetic_op(const exp_operator op)
{
	/*

	get_compound_arithmetic_op
	Gets the corresponding arithmetic operator for a compound one.

	For example, turns "+=" into "+"

	*/

	switch (op)
	{
	case PLUS_EQUAL:
		return PLUS;
	case MINUS_EQUAL:
		return MINUS;
	case MULT_EQUAL:
		return MULT;
	case DIV_EQUAL:
		return DIV;
	case MOD_EQUAL:
		return MODULO;
	case AND_EQUAL:
		return BIT_AND;
	case OR_EQUAL:
		return BIT_OR;
	case XOR_EQUAL:
		return BIT_XOR;
	case LEFT_SHIFT_EQUAL:
		return LEFT_SHIFT;
	case RIGHT_SHIFT_EQUAL:
		return RIGHT_SHIFT;
	
	default:
		return NO_OP;
	}
}
