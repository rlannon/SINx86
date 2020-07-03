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
        auto p = fetch_destination_operand(*sym, symbols, line, r, is_initialization);
        dest = p.first;
        gen_code << p.second;

        // marks the symbol as initialized
        sym->set_initialized();
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
            dest = "[rbx]";
            member_selection m = member_selection::create_member_selection(*lhs, structures, symbols, line);
            gen_code << m.evaluate(symbols, structures, line).str();
            
            // mark the symbol as initialized
            m.last().set_initialized();
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

std::pair<std::string, std::string> assign_utilities::fetch_destination_operand(
    symbol &sym,
    symbol_table &symbols,\
    unsigned int line,
    reg r,
    bool is_initialization
) {
    /*

    fetch_destination_operand
    An overloaded function to fetch symbol destinations

    */

    std::string dest;
    std::stringstream gen_code;

    auto dt = sym.get_data_type();

    // ensure we can actually assign to it
    if (sym.get_symbol_type() != SymbolType::VARIABLE) {
        throw InvalidSymbolException(line);
    }
    else if (dt.get_qualities().is_const() && !is_initialization) {
        throw ConstAssignmentException(line);
    }
    else if (dt.get_qualities().is_final() && sym.was_initialized()) {
        throw FinalAssignmentException(line);
    }
    else {
        if (dt.get_qualities().is_static()) {
            dest = "[rbx]";
            gen_code << "\t" << "lea rbx, [" << sym.get_name() << "]" << std::endl;
        }
        else {
            // we will need the stack location no matter what
            std::string location;
            if (sym.get_offset() < 0) {
                location = "[rbp + " + std::to_string(-sym.get_offset()) + "]";
            }
            else {
                location = "[rbp - " + std::to_string(sym.get_offset()) + "]";
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

    return std::make_pair<>(dest, gen_code.str());
}

bool assign_utilities::requires_copy(DataType &t) {
    /*

    requires_copy
    Returns whether the given type requires a copy routine or can be assigned directly

    */

    return (
        t.get_primary() == STRING ||
        t.get_primary() == ARRAY ||
        t.get_primary() == STRUCT
    );
}
