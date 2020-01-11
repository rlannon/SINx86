/*

SIN Toolchain (x86 target)
parse_definition.cpp
Copyright 2020 Riley Lannon

Contains the implementation of the parser's functions to construct definitions for structs and functions

*/

#include "Parser.h"

std::shared_ptr<Statement> Parser::parse_definition(lexeme current_lex) {
	/*

	parse_definition
	Parses a definition statement (function or struct)

	When the compiler sees a 'def' keyword, it knows it needs to construct a definition for something. This function delegates appropriately, as the keyword is used for functions and structs alike.
	Note that this function should begin with the lexeme pointer on 'def'; it begins parsing on the first token of the type data

	@param	current_lex	The current lexeme being examined by the parser
	@return	A shared_ptr containing the statement parsed

	*/
	
	// We will know where to delegate based on the next lexeme
	lexeme type_lex = this->next();

	// if the value is "struct", delegate to the struct; else, 
	if (type_lex.value == "struct") {
		return this->parse_struct_definition(type_lex);
	} else {
		return this->parse_function_definition(type_lex);
	}
}

std::shared_ptr<Statement> Parser::parse_function_definition(lexeme current_lex) {
	/*

	parse_function_definition
	Parses a function definition, returning a shared_ptr to the constructed object

	Note this function must begin parsing on the _first_ lexeme of the type data. It should be called only by 'parse_definition'

	@param	current_lex	The lexeme to begin parsing on
	@return	A shared_ptr to the statement containing the definition

	*/

	std::shared_ptr<Statement> stmt;

	// We are already on the first keyword of the type data
	DataType func_type_data = this->get_type();

	// Get the function name and verify it is of the correct type
	lexeme func_name = this->next();
	if (func_name.type == "ident") {
		lexeme _peek = this->peek();
		if (_peek.value == "(") {
			this->next();
			// Create our arguments vector and our StatementBlock variable
			StatementBlock procedure;
			std::vector<std::shared_ptr<Statement>> args;
			// Populate our arguments vector if there are arguments
			if (this->peek().value != ")") {
				this->next();
				while (this->current_token().value != ")") {
					args.push_back(this->parse_statement());
					this->next();

					// if we have multiple arguments, current_token() will return a comma, but we don't want to advance twice in case we hit the closing paren; as a result, we only advance once more if there is a comma
					if (this->current_token().value == ",") {
						this->next();
					}
				}
			}
			else {
				this->next();	// skip the closing paren
			}

			// Args should be empty if we don't have any
			// Now, check to make sure we have a curly brace
			if (this->peek().value == "{") {
				this->next();

				// if we have an empty definition, print a warning but continue parsing
				if (this->peek().value != "}") {
					this->next();	// if the definition isn't empty we can skip ahead, but we don't want to if it is (it will cause the parser to crash)
				}
				else {
					parser_warning("Empty function definition", this->current_token().line_number);	// print a warning and don't advance the token pointer
				}

				procedure = this->create_ast();
				this->next();	// skip closing curly brace

				// check to see if 'procedure' has a return statement using has_return
				bool returned = has_return(procedure);

				// if so, return it; otherwise, throw an error
				if (returned) {
					// Return the pointer to our function
					stmt = std::make_shared<FunctionDefinition>(func_name.value, func_type_data, args, std::make_shared<StatementBlock>(procedure));
					stmt->set_line_number(current_lex.line_number);

					return stmt;
				}
				else {
					throw ParserException("All functions must return a value (if type is void, use 'return void')", 0, current_lex.line_number);
				}
			}
			else {
				throw ParserException("Function definition requires use of curly braces after arguments", 331, current_lex.line_number);
			}
		}
		else {
			throw ParserException("Function definition requires '(' and ')'", 331, current_lex.line_number);
		}
	}
	// if NOT "ident"
	else {
		throw ParserException("Expected identifier", 330, current_lex.line_number);
	}
}

std::shared_ptr<Statement> Parser::parse_struct_definition(lexeme current_lex) {
    /*

    parse_struct_definition
    Parses a struct definition

    Note this function, like parse_function_definition, must begin on the first lexeme of the type information (on the keyword "struct")

    @param  current_lex The lexeme that begins the definition
    @return A shared_ptr to the parsed statement

    */

    std::shared_ptr<Statement> stmt;
    StatementBlock procedure;

    lexeme struct_name = this->next();
    if (struct_name.type == "ident") {
        // The next lexeme should be a curly brace
        if (this->peek().value == "{") {
            this->next();   // eat the curly brace
            
            // if we have an empty struct definition, continue parsing, but don't advance the token counter
            if (this->peek().value == "}") {
                parser_warning("Empty struct definition", this->current_token().line_number);
            } else {
                this->next();   // advance to the first token of the block
            }

            // parse the struct definition
            procedure = this->create_ast();
            this->next();   // skip the closing curly brace
        } else {
            throw ParserException("Expected scoped block in struct definition", 0, this->peek().line_number);
        }
    } else {
        throw ParserException("Expected identifier for struct name", 0, struct_name.line_number);
    }

    // construct the struct definition and return it
    stmt = std::make_shared<StructDefinition>(struct_name.value, std::make_shared<StatementBlock>(procedure));
    stmt->set_line_number(current_lex.line_number);
    return stmt;
}
