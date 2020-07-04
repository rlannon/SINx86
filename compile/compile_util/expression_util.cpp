/*

SIN toolchain (x86 target)
expression_util.cpp
Copyright 2020 Riley Lannon

Implements the expression utility functions given in the associated header

*/

#include "expression_util.h"

std::stringstream expression_util::get_exp_address(
    std::shared_ptr<Expression> exp,
    symbol_table &symbols,
    struct_table &structs,
    reg r,
    unsigned int line
) {
    /*

    Gets the address of an expression

    */

    std::stringstream addr_ss;

    std::string r_name = register_usage::get_register_name(r);

    if (exp->get_expression_type() == LVALUE) {
        // we have a utility for these already
        auto l = dynamic_cast<LValue*>(exp.get());
        auto sym = symbols.find(l->getValue());
        addr_ss << get_address(*sym, r);
    }
    else if (exp->get_expression_type() == UNARY) {
        // use this function recursively to get the address of the operand
        auto u = dynamic_cast<Unary*>(exp.get());
        addr_ss << get_exp_address(u->get_operand(), symbols, structs, r, line).str();
        
        if (u->get_operator() == DEREFERENCE) {
            // dereference the pointer we fetched
            addr_ss << "\t" << "mov " << r_name << ", [" << r_name << "]" << std::endl;
        }
    }
    else if (exp->get_expression_type() == INDEXED) {
        // use recursion
        auto i = dynamic_cast<Indexed*>(exp.get());
        addr_ss << get_exp_address(i->get_to_index(), symbols, structs, r, line).str();
        // note that this can't evaluate the index; that's up to the caller
    }
    else if (exp->get_expression_type() == BINARY) {
        // create and evaluate a member_selection object
        auto b = dynamic_cast<Binary*>(exp.get());
        member_selection m = member_selection::create_member_selection(*b, structs, symbols, line);
        addr_ss << m.evaluate(symbols, structs, line).str();

        // make sure the address goes into the register we selected (the 'evaluate' function returns the address of the member RBX)
        if (r != RBX) {
            addr_ss << "\t" << "mov " << r_name << ", rbx" << std::endl;
        }
    }

    return addr_ss;
}
