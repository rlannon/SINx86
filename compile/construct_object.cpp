/*

SIN toolchain (x86 target)
construct_object.cpp
Copyright 2021 Riley Lannon

Code for object construction

*/

#include "compiler.h"
#include "compile_util/construct.h"

std::string compiler::construct_object(const ConstructionStatement& s)
{
    /*

    construct_object
    Constructs an object according to the construct statement

    The thing is, the expression after the 'construct' should be an
    identifier, which could be a structure name or an object name.
    Thus, we have to try both.

    */

    // todo: extract this into a function

    const symbol *to_construct_symbol = nullptr;
    const struct_info *to_construct_type = nullptr;
    std::string lookup_name;

    if (s.get_to_construct().get_expression_type() == IDENTIFIER)
    {
        const auto& exp = static_cast<const Identifier&>(s.get_to_construct());
        lookup_name = exp.getValue();
    }
    else
    {
        // todo: what else could it be?
        throw CompilerException (
            "Invalid Expression type in construction",
            compiler_errors::INVALID_EXPRESSION_TYPE_ERROR,
            s.get_line_number()
        );
    }

    // now that we have the name to look up, fetch whatever data we can
    try
    {
        to_construct_type = &this->get_struct_info(
            lookup_name,
            s.get_line_number()
        );
    }
    catch(const UndefinedException& e)
    {
        try
        {
            to_construct_symbol = this->lookup(
                lookup_name,
                s.get_line_number()
            );

            to_construct_type = &this->get_struct_info(
                to_construct_symbol->get_data_type().get_struct_name(),
                s.get_line_number()
            );
        }
        catch(const SymbolNotFoundException& e)
        {
            throw CompilerException(
                "Unknown identifier '" + lookup_name + "' in construction",
                compiler_errors::UNDEFINED_ERROR,
                s.get_line_number()
            );
        }
        catch (const UndefinedException& e)
        {
            throw CompilerException(
                "Structure required in construction statements",
                compiler_errors::TYPE_ERROR,
                s.get_line_number()
            );
        }
    }

    /*

    Before we actually perform any code generation, we need to make sure
    the statement is valid. If there is a default _and_ not all members are
    initialized, we are OK. Otherwise, if there is no default, the number
    of members in the construction must be equal to the number of members
    actually in the struct.

    */

    if (!construct_util::is_valid_construction(s, *to_construct_type))
    {
        throw CompilerException(
            "Unexpected number of initializations in construction",
            compiler_errors::CONSTRUCTION_NUMBER_INIT_ERROR,
            s.get_line_number()
        );
    }

    /*

    Now, perform the code generation

    */

    std::stringstream construct_ss;

    // the first thing we need to go is get the address of the
    // object in question; that will allow us to get all addresses
    if (to_construct_symbol)
    {
        construct_ss << expression_util::get_exp_address(
            s.get_to_construct(),
            this->symbols,
            this->structs,
            RBX,
            s.get_line_number()
        ).str();
    }
    else if (to_construct_type)
    {
        // Create space on the stack for this anonymous object
        construct_ss << "\t" << "sub rsp, " << to_construct_type->get_width() << std::endl;
        construct_ss << "\t" << "mov rbx, rsp" << std::endl;
        this->max_offset += to_construct_type->get_width();
    }
    else
    {
        throw UndefinedException(s.get_line_number());
    }
    
    construct_ss << generate_construction(  s.get_construction(),
                                            to_construct_type,
                                            s.get_line_number(),
                                            RBX );
    
    return construct_ss.str();
}

std::string compiler::generate_construction(const Construction &construction_expression,
                                            const struct_info *to_construct_type,
                                            const unsigned int line,
                                            reg r)
{
    /*

    generate_construction
    Generates code for a construction statement, storing the results in the data pointed to by _r_.

    */

    std::stringstream construct_ss;

    // iterate over defined elements
    for (const auto& elem: construction_expression.get_initializers())
    {
        // todo: evaluate and initialize each member in order

        auto p = evaluate_expression(elem.get_value(), line);
        construct_ss << p.first;

        // the result is in RBX
        // fetch the member offset
        std::string member_name;
        if (elem.get_member().get_expression_type() == IDENTIFIER)
        {
            member_name = static_cast<const Identifier&>(elem.get_member()).getValue();
        }
        else
        {
            // todo: what else could it be?
            throw CompilerException(
                "Invalid expression for construction of member",
                compiler_errors::TYPE_ERROR,
                line
            );
        }

        // now, utilize "mov [rbx + <offset>], <rax name>"
        const symbol *member = to_construct_type->get_member(member_name);
        if (member)
        {
            construct_ss << "\t" << 
                "mov [rbx + " << member->get_offset() << "], " << 
                register_usage::get_register_name(RAX, member->get_data_type()) << std::endl;
        }
        else
        {
            throw CompilerException(
                "Unknown struct member '" + member_name + "'",
                compiler_errors::SYMBOL_NOT_FOUND_ERROR,
                line
            );
        }

        // todo: what to do with p.second?
    }
    
    // todo: handle 'default'

    // todo: finish codegen

    return construct_ss.str();
}
