/*

assign_util.cpp
Copyright 2020 Riley Lannon

*/

#include "assign_util.h"

std::pair<std::string, std::string> assign_utilities::fetch_destination_operand(
    std::shared_ptr<Expression> exp,
    symbol_table &symbols,
    struct_table &structures,
    std::string scope_name,
    unsigned int scope_level,
    unsigned int line,
    reg r,
    bool is_initialization
) {
    /*

    fetch_destination_operand
    Fetches the destination for the given expression
    
    This function returns both the location and the code to fetch it in a pair<string, string>

    */

    // todo: allow register to be modified (currently, r is an unused parameter)

    std::string dest;
    std::stringstream gen_code;

    // generate code based on the expression type
    if (exp->get_expression_type() == LVALUE) {
        // get the symbol information
        auto lhs = dynamic_cast<LValue*>(exp.get());
        auto sym = symbols.find(lhs->getValue());
        auto dt = sym->get_data_type();

        // ensure we can actually assign to it
        if (dt.get_qualities().is_const() && !is_initialization) {
            throw ConstAssignmentException(line);
        }
        else if (dt.get_qualities().is_final() && sym->was_initialized()) {
            throw FinalAssignmentException(line);
        }
        else {
            if (dt.get_qualities().is_static()) {
                dest = "[rbx]";
                gen_code << "\t" << "lea rbx, [" << sym->get_name() << "]" << std::endl;
            }
            else {
                // we will need the stack location no matter what
                std::string location;
                if (sym->get_offset() < 0) {
                    location = "[rbp + " + std::to_string(-sym->get_offset()) + "]";
                }
                else {
                    location = "[rbp - " + std::to_string(sym->get_offset()) + "]";
                }

                if (dt.get_qualities().is_dynamic() || dt.get_primary() == STRING) {
                    dest = "[rbx]";
                    gen_code << "\t" << "mov rbx, " << location << std::endl;
                }
                else {
                    dest = location;
                }
            }
        }
    }
    else if (exp->get_expression_type() == UNARY) {
        // get the unary
        auto lhs = dynamic_cast<Unary*>(exp.get());
        if (lhs->get_operator() == exp_operator::DEREFERENCE) {
            auto fetched = fetch_destination_operand(
                lhs->get_operand(),
                symbols,
                structures,
                scope_name,
                scope_level,
                line,
                r,
                is_initialization
            );
            
            // add the fetched code from the recursive call to this one
            dest = "[rbx]";
            gen_code << fetched.second;
            
            // now, add an instruction to move the previously fetched destination into RBX
            gen_code << "\t" << "mov rbx, " << fetched.first << std::endl;
        }
        else {
            throw NonModifiableLValueException(line);
        }
    }
    else if (exp->get_expression_type() == BINARY) {
        auto lhs = dynamic_cast<Binary*>(exp.get());
        if (lhs->get_operator() == DOT) {
            // todo: binaries
        }
        else {
            throw NonModifiableLValueException(line);
        }
    }
    else if (exp->get_expression_type() == INDEXED) {
        auto lhs = dynamic_cast<Indexed*>(exp.get());
        // todo: indexed expressions
    }
    else {
        throw NonModifiableLValueException(line);
    }

    return std::make_pair<>(dest, gen_code.str());
}
