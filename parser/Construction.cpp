/*

SIN toolchain (x86 target)
Construction.cpp
Copyright 2021 Riley Lannon

This file contains the implementation for functions that parse construction statements and expressions.

*/

#include "Parser.h"

std::unique_ptr<Statement> Parser::parse_construction()
{
    /*

    parse_construction
    Parses a construction statement

    */

    this->next();
    auto to_construct = this->parse_expression(true);   // allow a brace after this expression
    auto body = this->parse_construction_body();

    // ensure we end with a curly brace
    if (this->peek().value == "}")
    {
        this->next();
    }
    else
    {
        throw ParserException(
            "Expected closing curly brace",
            compiler_errors::MISSING_GROUPING_SYMBOL_ERROR,
            this->current_token().line_number
        );
    }

    return std::make_unique<ConstructionStatement>(std::move(to_construct), std::move(body));
}

std::unique_ptr<Construction> Parser::parse_construction_body(const bool has_type, const std::string& explicit_type)
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
        bool has_default = false;
        bool is_const = true;   // assume it can be deduced at compile-time (all expressions must be for this to remain true)

        while (this->peek().type != KEYWORD_LEX && this->peek().value != "}")
        {
            // Expression parsing begins on the first token of the expression
            this->next();
            auto member = this->parse_expression(0UL, "(", false, true);    // all defaults except 'omit_equals'
            
            // we should see a colon, then the initialization expression
            if (this->peek().value == ":")
            {
                this->next();   // skip colon; skip ahead to first character of initialization
                this->next();
                
                auto value = this->parse_expression();
                is_const = is_const && value->is_const();

                initializers.push_back(
                    Construction::Constructor{member, value}
                );

                // we are now on the last lexeme of the expression; the next should be a comma
                if (this->peek().value == ",")
                {
                    this->next();
                }
                else if (this->peek().value != "}")
                {
                    throw ParserException(
                        "Expected commas between expressions",
                        compiler_errors::EXPECTED_LIST_INITIALIZATION,
                        this->current_token().line_number
                    );
                }
            }
            else
            {
                throw ParserException("Expected initialization", compiler_errors::EXPECTED_INITIALIZATION, this->peek().line_number);
            }
        }

        // we may have ended in 'default'
        if (this->peek().value == "default")
        {
            this->next();
            has_default = true;
            
            // we are allowed to have a comma after the last expression
            if (this->peek().value == ",")
            {
                this->next();
            }

            // we must end the expression here
            if (this->peek().value != "}")
            {
                throw ParserException(
                    "Expected closing curly brace after 'default",
                    compiler_errors::MISSING_GROUPING_SYMBOL_ERROR,
                    this->current_token().line_number
                );
            }
        }

        auto ctor = std::make_unique<Construction>(std::move(initializers));
        if (has_type)
            ctor->set_explicit_type(explicit_type);
        
        if (has_default)
            ctor->set_default();
        
        if (is_const)
            ctor->set_const();
        
        return ctor;
    }
    else
    {
        throw ParserException(
            "Expected a block",
            compiler_errors::MISSING_GROUPING_SYMBOL_ERROR,
            this->current_token().line_number
        );
    }
}
