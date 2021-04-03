/*

SIN toolchain (x86 target)
Construction.cpp
Copyright 2021 Riley Lannon

This file contains the implementation for functions that parse construction statements and expressions.

*/

#include "Parser.h"

std::unique_ptr<Statement> Parser::parse_construction(const lexeme& current_lex)
{
    /*

    parse_construction
    Parses a construction statement

    */



    return nullptr;
}

std::unique_ptr<Construction> Parser::parse_construction_body(const lexeme& current_lex)
{
    /*

    parse_construction_body
    Parses the body of a construction expression.

    This may be used standalone to construct a construction expression or a construction statement.

    */

    if (this->next().value == "{")
    {
        /*

        Continue parsing expressions while the peeked token is not keyword or a closing curly brace

        */

        std::vector<Construction::Constructor> initializers;
        while (this->peek().type != KEYWORD_LEX && this->peek().value != "}")
        {
            // Expression parsing begins on the first token of the expression
            this->next();
            auto member = this->parse_expression();
        }
    }
    else
    {
        throw ParserException(
            "Expected a block",
            compiler_errors::MISSING_GROUPING_SYMBOL_ERROR,
            current_lex.line_number
        );
    }
}
