/*

SIN Toolchain
ParseStatement.cpp
Copyright 2019 Riley Lannon

The implementation of the Parser member functions to parse statements, including:
	- std::shared_ptr<Statement> parse_statement();	// entry function to parse a statement
	- std::shared_ptr<Statement> parse_include(lexeme current_lex);
	- std::shared_ptr<Statement> parse_declaration(lexeme current_lex);
	- std::shared_ptr<Statement> parse_ite(lexeme current_lex);
	- std::shared_ptr<Statement> parse_allocation(lexeme current_lex);
	- std::shared_ptr<Statement> parse_assignment(lexeme current_lex);
	- std::shared_ptr<Statement> parse_return(lexeme current_lex);
	- std::shared_ptr<Statement> parse_while(lexeme current_lex);
	- std::shared_ptr<Statement> parse_definition(lexeme current_lex);
	- std::shared_ptr<Statement> parse_function_call(lexeme current_lex);

Keeping the statement parsing functions together makes for a smaller 'Parser.cpp' file and more readable code.

*/

#include "Parser.h"

std::shared_ptr<Statement> Parser::parse_statement(bool is_function_parameter) {
	// get our current lexeme and its information so we don't need to call these functions every time we need to reference it
	lexeme current_lex = this->current_token();

	// create a shared_ptr to the statement we are going to parse so that we can return it when we are done
	std::shared_ptr<Statement> stmt = nullptr;

	// first, we will check to see if we need any keyword parsing
	if (current_lex.type == KEYWORD_LEX) {

		// Check to see what the keyword is

		// parse an "include" directive
		if (current_lex.value == "include") {
			stmt = this->parse_include(current_lex);
		}
		// parse inline assembly
		else if (current_lex.value == "asm") {
			if (this->peek().value == "{") {
				this->next();

				// TODO: figure out how to implement line breaks in asm

				// continue reading values into a stringstream until we hit another curly brace
				bool end_asm = false;
				std::stringstream asm_code;

				lexeme asm_data = this->next();
				unsigned int current_line = asm_data.line_number;
				
				while (!end_asm) {
					// if we have advanced in line number, add a newline 
					if (asm_data.line_number > current_line) {
						asm_code << std::endl;
						current_line = asm_data.line_number;	// update 'current_line' -- if we advance lines from here, we will add a newline at the top of the next loop
					}

					if (asm_data.value == "}") {
						end_asm = true;
					}
					else {
						asm_code << asm_data.value;

						// put a space after idents, but not if a colon follows; also put spaces before semicolons
						if (((asm_data.type == IDENTIFIER_LEX) && (this->peek().value != ":")) || (this->peek().value == ";")) {
							asm_code << " ";
						}
						// advance to the next token, but ONLY if we haven't hit the closing curly brace
						asm_data = this->next();
					}
				}

				stmt = std::make_shared<InlineAssembly>(asm_code.str());
				stmt->set_line_number(current_lex.line_number);	// sets the line number for errors to the ASM block start; any ASM errors will be made known in the assembler
			}
		}
		// parse a "free" statement
		else if (current_lex.value == "free") {
			/*

			The syntax for a free statement is:
				free <expression>;
			
			Since free is not a function, but a keyword, unlike C, it is more like the C++ 'delete'

			*/

			this->next();
			auto to_free = this->parse_expression();
			stmt = std::make_shared<FreeMemory>(to_free);
			stmt->set_line_number(current_lex.line_number);
		}
		// parse a declaration
		else if (current_lex.value == "decl") {
			stmt = this->parse_declaration(current_lex, is_function_parameter);
		}
		// parse an ITE
		else if (current_lex.value == "if") {
			stmt = this->parse_ite(current_lex);
		}
		// pare an allocation
		else if (current_lex.value == "alloc") {
			stmt = this->parse_allocation(current_lex, is_function_parameter);
		}
		// Parse an assignment
		else if (current_lex.value == "let") {
			stmt = this->parse_assignment(current_lex);
		}
		else if (current_lex.value == "move") {
			stmt = this->parse_move(current_lex);
		}
		// Parse a return statement
		else if (current_lex.value == "return") {
			stmt = this->parse_return(current_lex);
		}
		// Parse a 'while' loop
		else if (current_lex.value == "while") {
			stmt = this->parse_while(current_lex);
		}
		// Parse a definition -- could be function or struct, call the delegator
		else if (current_lex.value == "def") {
			stmt = this->parse_definition(current_lex);
		}
		else if (current_lex.value == "pass") {
			this->next();
			stmt = std::make_shared<Statement>(STATEMENT_GENERAL, current_lex.line_number);	// an explicit pass will, essentially, be ignored by the compiler; it does nothing
		}
		// if none of the keywords were valid, throw an error
		else {
			throw ParserException("Invalid keyword", 211, current_lex.line_number);
		}

	}
	// if it's not a keyword, check to see if we need to parse a function call
	else if (current_lex.type == OPERATOR) {	// "@" is an op_char, but we may update the lexer to make it a "control_char"
		if (current_lex.value == "@") {
			stmt = this->parse_function_call(current_lex);
		}
		else {
			throw ParserException(
                "Lexeme '" + current_lex.value + "' is not a valid beginning to a statement",
                000,
                current_lex.line_number
            );
		}
	}
	// otherwise, we might have a scoped block statement
	else if (current_lex.value == "{") {
		// eat the curly brace
		this->next();
		StatementBlock scope_ast = this->create_ast();
		this->next();	// eat the closing curly brace
		// NB: scope blocks never need semicolons

		// create the statement
		stmt = std::make_shared<ScopedBlock>(scope_ast);
	}

	// if it is a curly brace, advance the character and return a nullptr; the compiler will skip this
	else if (current_lex.value == "}") {
		this->next();
		stmt = std::make_shared<Statement>(STATEMENT_GENERAL, current_lex.line_number);
	}

	// otherwise, if the lexeme is not a valid beginning to a statement, abort
	else {
		throw ParserException("Lexeme '" + current_lex.value + "' is not a valid beginning to a statement", 000, current_lex.line_number);
	}

	return stmt;
}

std::shared_ptr<Statement> Parser::parse_include(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt = nullptr;

	lexeme next = this->next();

	if (next.type == STRING_LEX) {
		std::string filename = next.value;

		stmt = std::make_shared<Include>(filename);
		stmt->set_line_number(current_lex.line_number);
	}
	else {
		throw ParserException("Expected a filename in quotes in 'include' statement", 0, current_lex.line_number);
		// TODO: error numbers for includes
	}

	return stmt;
}

std::shared_ptr<Statement> Parser::parse_declaration(lexeme current_lex, bool is_function_parameter) {
	/*

	Parse a declaration statement. Appropriate syntax is:
		decl <type> <name>;
	or
		decl <type> <name>(<formal parameters>);
	where the formal parameters are also declarations, or
		decl struct <name>;

	*/

	lexeme next_lexeme = this->next();
	std::shared_ptr<Expression> initial_value = nullptr;
	std::shared_ptr<Declaration> stmt = nullptr;

	// the next lexeme must be a keyword (specifically, a type or 'struct')
	if (next_lexeme.value == "struct") {
		// struct declaration
		if (this->peek().type == IDENTIFIER_LEX) {
			DataType struct_type(
				STRUCT,
				NONE,
				symbol_qualities(),
				nullptr,
				this->next().value
			);
			stmt = std::make_shared<Declaration>(struct_type, "", initial_value, false, true);
		}
		else {
			throw CompilerException("Expected struct name", compiler_errors::ILLEGAL_STRUCT_NAME, this->current_token().line_number);
		}
	}
	else if (next_lexeme.type == KEYWORD_LEX) {
		DataType symbol_type_data = this->get_type();
		
		// get the variable name
		next_lexeme = this->next();
		if (next_lexeme.type == IDENTIFIER_LEX) {		// variable names must be identifiers; if an identifier doesn't follow the type, we have an error
			// get our variable name
			std::string var_name = next_lexeme.value;
			bool is_function = false;

			// check to see if we have postfixed symbol qualities
			if (this->peek().value == "&") {
				// append the posftixed qualities to symbol_type_data.qualities
				this->next();
				symbol_qualities postfixed_qualities = this->get_postfix_qualities(is_function_parameter ? "(" : "");
				try {
					symbol_type_data.add_qualities(postfixed_qualities);
				} catch(std::string &offending_quality) {
					throw QualityConflictException(offending_quality, this->current_token().line_number);
				}
			}

			std::vector<std::shared_ptr<Statement>> formal_parameters = {};

			// next, check to see if we have a paren following the name; if so, it's a function, so we need to get the formal parameters
			if (this->peek().value == "(") {
				// if there is a paren, it's a function declaration
				is_function = true;

				// todo: parse function declarations
				this->next();	// eat the opening paren

				// so long as we haven't hit the end of the formal parameters, continue parsing
				while (this->peek().value != ")") {
					this->next();
					std::shared_ptr<Statement> next = this->parse_statement(true);

					// the statement _must_ be a declaration, not an allocation
					if (next->get_statement_type() == DECLARATION) {
						formal_parameters.push_back(next);
					}
					else {
						throw ParserException("Definitions of formal parameters in a declaration of a function must use 'decl' (not 'alloc'", 0,
							this->current_token().line_number);
					}

					if (this->peek().value == ",") {
						this->next();
					}
				}

				this->next();	// eat the closing paren
			}
			// otherwise, if the name is followed by a colon, we have a default value
			else if (this->peek().value == ":") {
				// however, we may only use alloc-assign syntax if the 'decl' is part of a function parameter
				if (is_function_parameter) {
					this->next();
					this->next();	// parse_expression(...) begins on the _first token_ of the expression
					initial_value = this->parse_expression();
				}
				else {
					throw CompilerException("Cannot use alloc-assign syntax in declarations unless said declaration is a default function parameter",
						this->current_token().line_number);
				}
			}
			
			// finally, we must have a semicolon, a comma, or a closing paren
			if (this->peek().value == ";" || this->peek().value == "," || this->peek().value == ")") {
				Declaration decl_statement(symbol_type_data, var_name, initial_value, is_function, false, formal_parameters);
				decl_statement.set_line_number(next_lexeme.line_number);

				stmt = std::make_shared<Declaration>(decl_statement);
			}
			else if (this->peek().value == ":") {
				throw ParserException("Initializations are forbidden in declaration statements", 0, next_lexeme.line_number);
			}
			else {
				throw MissingSemicolonError(next_lexeme.line_number);
			}
		}
		else {
			throw ParserException("Expected variable name after type in Declaration", 0, next_lexeme.line_number);
		}
	}
	// if it is not followed by a type name, throw an exception
	else {
		throw ParserException("Expected type name following 'decl' in variable declaration", 0, current_lex.line_number);
	}

	return stmt;
}

std::shared_ptr<Statement> Parser::parse_ite(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt = nullptr;

	// Get the next lexeme
	lexeme next = this->next();

	// Check to see if condition is enclosed in parens
	if (next.value == "(") {
		// get the condition
		this->next();
		std::shared_ptr<Expression> condition = this->parse_expression();

		if (this->peek().value == ")")
			this->next();
		else
			throw CompilerException("Expected ')' in conditional", compiler_errors::MISSING_GROUPING_SYMBOL_ERROR, this->current_token().line_number);
		
		// Initialize the if_block
		std::shared_ptr<Statement> if_branch;
		std::shared_ptr<Statement> else_branch;
		
		// create the branch
		this->next();	// skip ahead to the first character of the statement
		if_branch = this->parse_statement();

		// if there was a single statement, ensure there was a semicolon
		if (this->peek().value == ";")
			this->next();
		else {
			if (this->current_token().value != "}")
				throw MissingSemicolonError(this->current_token().line_number);
		}

		// Check for an else clause
		if (!this->is_at_end() && this->peek().value == "else") {
			// if we have an else clause
			this->next();	// skip the keyword
			this->next();	// skip ahead to the first token in the statment

			// parse the statement
			else_branch = this->parse_statement();

			// construct the statement and return it
			stmt = std::make_shared<IfThenElse>(condition, if_branch, else_branch);
		}
		else {
			// if we do not have an else clause, we will return the if clause alone here
			stmt = std::make_shared<IfThenElse>(condition, if_branch);
		}

		stmt->set_line_number(current_lex.line_number);
	}
	// If condition is not enclosed in parens
	else {
		throw ParserException("Condition must be enclosed in parens", 331, current_lex.line_number);
	}

	return stmt;
}

std::shared_ptr<Statement> Parser::parse_allocation(lexeme current_lex, bool is_function_parameter)
{
	// create an object for the statement as well as the variable's name
	std::shared_ptr<Statement> stmt;
	std::string new_var_name = "";

	// check our next token; it must be a keyword or a struct name (ident)
	lexeme next_token = this->next();
	if (next_token.type == KEYWORD_LEX || next_token.type == IDENTIFIER_LEX) {
		// get the type data using Parser::get_type() -- this will tell us if the memory is to be dynamically allocated
		// It will also throw an exception if the type specifier was invalid
		DataType symbol_type_data = this->get_type();

		// next, get the name
		if (this->peek().type == IDENTIFIER_LEX) {
			next_token = this->next();
			new_var_name = next_token.value;

			// get our postfixed qualities, if we have any
			// now, get postfixed symbol qualities, if we have any
			if (this->peek().value == "&") {
				// append the posftixed qualities to symbol_type_data.qualities
				this->next();
				symbol_qualities postfixed_qualities = this->get_postfix_qualities(is_function_parameter ? "(" : "");

				// we may encounter an error when trying to add our postfixed qualities; catch it and craft a new exception that includes the line number
				try {
					symbol_type_data.add_qualities(postfixed_qualities);
				} catch (std::string &offending_quality) {
					throw QualityConflictException(offending_quality, this->current_token().line_number);
				}
			}

			bool initialized = false;
			std::shared_ptr<Expression> initial_value = std::make_shared<Expression>();

			// the name can be followed by a semicolon, a comma, a closing paren, or a colon
			// if it's a colon, we have an initial value
			if (this->peek().value == ":") {
				this->next();
				this->next();	// advance the iterator so it points to the first character of the expression
				initialized = true;
				initial_value = this->parse_expression();
			}

			// if it's a semicolon, comma, or closing paren, craft the statement and return
			if (this->peek().value == ";" || this->peek().value == "," || this->peek().value == ")") {
				// craft the statement
				stmt = std::make_shared<Allocation>(symbol_type_data, new_var_name, initialized, initial_value);
				stmt->set_line_number(next_token.line_number);	// set the line number
			}
			// otherwise, it's an invalid character
			else {
				throw MissingSemicolonError(this->current_token().line_number);
			}
		}
		else {
			throw ParserException(
                "The variable's type must be followed by a valid identifier",
                compiler_errors::MISSING_IDENTIFIER_ERROR,
                next_token.line_number
            );
		}
	} else {
		throw ParserException("Expected a valid data type", compiler_errors::TYPE_ERROR, current_lex.line_number);
	}

	return stmt;
}

std::shared_ptr<Statement> Parser::parse_assignment(lexeme current_lex)
{

	// Create a shared_ptr to our assignment expression
	std::shared_ptr<Assignment> assign;
	// Create an object for our left expression
	std::shared_ptr<Expression> lvalue;

	// parse an expression for our lvalue (the compiler will verify the type later)
	this->next();	// Parser::parse_expression must have the token pointer on the first token of the expression
	lvalue = this->parse_expression(0, "(", false, true);

	// now, "lvalue" should hold the proper variable reference for the assignment
	// get the operator character, make sure it's an equals sign
	lexeme op_lex = this->next();
	exp_operator op = translate_operator(op_lex.value);

	if (is_valid_copy_assignment_operator(op)) {
		// if the next lexeme is not a semicolon and the next lexeme's line number is the same as the current lexeme's line number, we are ok
		if ((this->peek().value != ";") && (this->peek().line_number == current_lex.line_number)) {
			// create a shared_ptr for our rvalue expression
			std::shared_ptr<Expression> rvalue;
			this->next();
			rvalue = this->parse_expression();

			if (op != EQUAL) {
				rvalue = Parser::create_compound_assignment_rvalue(lvalue, rvalue, op);
			}

			assign = std::make_shared<Assignment>(lvalue, rvalue);
			assign->set_line_number(current_lex.line_number);
			return assign;
		}
		// otherwise, we have a syntax error -- we didn't get an expression where we expected it
		else {
			throw ParserException("Expected expression", 0, current_lex.line_number);
		}
	}
	else if (is_valid_move_assignment_operator(op)) {
		throw ParserException("Move assignment operator not supported with 'let'", compiler_errors::OPERATOR_TYPE_ERROR, op_lex.line_number);
	}
	else {
		throw ParserException("Unrecognized token.", 0, current_lex.line_number);
	}
}

std::shared_ptr<Statement> Parser::parse_move(lexeme current_lex)
{
	/*

	parse_move
	Parses a 'move' statement

	*/

	std::shared_ptr<Statement> stmt = nullptr;
	this->next();
	
	// get the lhs
	auto lhs = this->parse_expression();
	
	// get the op
	auto op = this->read_operator(false);
	if (is_valid_move_assignment_operator(op)) {
		// get the rhs
		this->next();
		auto rhs = this->parse_expression();

		if (this->peek().value != ";") {
			throw MissingSemicolonError(this->current_token().line_number);
		}

		// todo: ensure that we are only moving modifiable-lvalues (should this be done in the compiler class?)
		
		if (op == LEFT_ARROW) {
			// rhs is rvalue (the value)
			stmt = std::make_shared<Movement>(lhs, rhs);
		}
		else {
			// lhs is rvalue (the value)
			stmt = std::make_shared<Movement>(rhs, lhs);
		}

        stmt->set_line_number(current_lex.line_number);
	}
	else {
		throw ParserException("Expected move assignment operator", compiler_errors::OPERATOR_TYPE_ERROR, current_lex.line_number);
	}

	return stmt;
}

std::shared_ptr<Statement> Parser::parse_return(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt = nullptr;
	this->next();	// go to the expression

	// if the current token is a semicolon, return a Literal Void
	if (this->current_token().value == ";" || this->current_token().value == "void") {
		// if we have "void", we need to skip ahead to the semicolon
		if (this->current_token().value == "void") {
			if (this->peek().value == ";") {
				this->next();
			}
			else {
				// we expect a semicolon after the return statement; throw an exception if there isn't one
				throw MissingSemicolonError(current_lex.line_number);
			}
		}

		// craft the statement
		stmt = std::make_shared<ReturnStatement>(std::make_shared<Literal>(VOID, "", NONE));
		stmt->set_line_number(current_lex.line_number);
	}
	// otherwise, we must have an expression
	else {
		// get the return expression
		std::shared_ptr<Expression> return_exp = this->parse_expression();

		// create a return statement from it and set the line number
		stmt = std::make_shared<ReturnStatement>(return_exp);
		stmt->set_line_number(current_lex.line_number);
	}

	// return the statement
	return stmt;
}

std::shared_ptr<Statement> Parser::parse_while(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt;

	// A while loop is very similar to an ITE in how we parse it; the only difference is we don't need to check for an "else" branch
	std::shared_ptr<Expression> condition;	// create the object for our condition
	StatementBlock branch;	// and for the loop body
	lexeme next = this->next();

	if (next.value == "(") {
		// get condition
		this->next();
		condition = this->parse_expression();
		
		if (this->peek().value == ")")
			this->next();
		else
		{
			throw CompilerException("Expected parentheses around conditional", compiler_errors::MISSING_GROUPING_SYMBOL_ERROR, this->current_token().line_number);
		}

		// initialize the branch
		std::shared_ptr<Statement> branch;

		// create the branch
		this->next();
		branch = this->parse_statement();

		// if there was a single statement, ensure there was a semicolon
		if (this->peek().value == ";")
			this->next();
		else if (this->current_token().value != "}")
			throw MissingSemicolonError(this->current_token().line_number);
		
		stmt = std::make_shared<WhileLoop>(condition, branch);
		stmt->set_line_number(current_lex.line_number);
	}
	else {
		throw ParserException("Expected a condition", 331, current_lex.line_number);
	}

	return stmt;
}

std::shared_ptr<Statement> Parser::parse_function_call(lexeme current_lex)
{
	std::shared_ptr<Statement> stmt = nullptr;
    auto parsed = this->parse_expression();
    if (parsed->get_expression_type() == CALL_EXP) {
        CallExpression *exp = static_cast<CallExpression*>(parsed.get());

        // if we didn't get a CallExpression, then it's an error -- we /must/ have one for a Call statement 
        // this means if we have a binary or something else (e.g., '@x.y().z'), it's not valid
        stmt = std::make_shared<Call>(*exp);
        stmt->set_line_number(current_lex.line_number);
    }
    else {
        throw ParserException(
            "Expected a valid function call expression",
            compiler_errors::INVALID_EXPRESSION_TYPE_ERROR,
            current_lex.line_number
        );
    }

    return stmt;
}
