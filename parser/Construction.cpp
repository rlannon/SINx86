/*

SIN toolchain (x86 target)
Construction.cpp
Copyright 2021 Riley Lannon

This file contains the implementation for functions that parse construction statements and expressions.

*/

#include "Parser.h"

std::unique_ptr<Statement> Parser::parse_construction(lexeme current_lex)
{
    /*

    parse_construction
    Parses a construction statement

    */

    return nullptr;
}

std::unique_ptr<Construction> Parser::parse_construction_body(lexeme current_lex)
{
    /*

    parse_construction_body
    Parses the body of a construction expression.

    This may be used standalone to construct a construction expression or a construction statement.

    */

    return nullptr;
}
