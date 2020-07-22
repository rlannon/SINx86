/*

assign_util.cpp
Copyright 2020 Riley Lannon

*/

#include "assign_util.h"

assign_utilities::destination_information::destination_information(
    std::string dest_location,
    std::string fetch_instructions,
    std::string address_for_lea,
    bool in_register
) {
    this->dest_location = dest_location;
    this->fetch_instructions = fetch_instructions;
    this->address_for_lea = address_for_lea;
    this->in_register = in_register;
}

assign_utilities::destination_information assign_utilities::fetch_destination_operand(
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
    std::string address_for_lea;
    bool in_register;

    // generate code based on the expression type
    if (exp->get_expression_type() == LVALUE) {
        // get the symbol information
        auto lhs = dynamic_cast<LValue*>(exp.get());
        auto sym = symbols.find(lhs->getValue());
        auto p = fetch_destination_operand(*sym, symbols, line, r, is_initialization);
        dest = p.dest_location;
        address_for_lea = p.address_for_lea;
        in_register = p.in_register;
        gen_code << p.fetch_instructions;

        // marks the symbol as initialized
        sym->set_initialized();
    }
    else if (exp->get_expression_type() == UNARY) {
        // get the unary
        auto lhs = dynamic_cast<Unary*>(exp.get());
        if (lhs->get_operator() == exp_operator::DEREFERENCE) {
            // ensure the expression has a pointer type; else, indirection is illegal
            auto op_t = get_expression_data_type(lhs->get_operand(), symbols, structures, line);
            if (op_t.get_primary() == PTR) {
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
                address_for_lea = fetched.dest_location;
                gen_code << fetched.fetch_instructions;
                in_register = fetched.in_register;
                
                // now, add an instruction to move the previously fetched destination into RBX
                gen_code << "\t" << "mov rbx, " << fetched.dest_location << std::endl;
            }
            else {
                throw IllegalIndirectionException(line);
            }
        }
        else {
            throw NonModifiableLValueException(line);
        }
    }
    else if (exp->get_expression_type() == BINARY) {
        auto lhs = dynamic_cast<Binary*>(exp.get());
        if (lhs->get_operator() == DOT) {
            dest = "[rbx]";
            // todo: address for lea with dot selection?
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
        /*

        Indexed expressions can't be evaluated properly by this utility
        As such, we should return the destination as [rbx] with no code generation
        The code to generate the actual assignment function

        */

        dest = "[rbx]";
    }
    else {
        throw NonModifiableLValueException(line);
    }
    
    return destination_information(dest, gen_code.str(), address_for_lea, in_register);
}

assign_utilities::destination_information assign_utilities::fetch_destination_operand(
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
    std::string address_for_lea;
    bool in_register = false;

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
            address_for_lea = "[" + sym.get_name() + "]";
            gen_code << "\t" << "lea rbx, [" << sym.get_name() << "]" << std::endl;
        }
        else {
            // we will need the stack location no matter what
            std::string location;
            if (sym.get_register() == NO_REGISTER) {
                if (sym.get_offset() < 0) {
                    location = "[rbp + " + std::to_string(-sym.get_offset()) + "]";
                }
                else {
                    location = "[rbp - " + std::to_string(sym.get_offset()) + "]";
                }
            }
            else {
                in_register = true;
                location = register_usage::get_register_name(sym.get_register(), sym.get_data_type());
            }

            // if we have a reference type, we need to dereference under the hood
            // this is, of course, unless we are *initializing* a ref<T>
            if (
                (dt.is_reference_type()) &&
                !(dt.get_primary() == REFERENCE && is_initialization)
            ) {
                dest = "[rbx]";
                gen_code << "\t" << "mov rbx, " << location << std::endl;
            }
            else if (requires_copy(sym.get_data_type())) {
                // if we don't have a string but we do require a copy, use lea
                // but if that value is in a register, just use mov
                dest = "[rbx]";
                if (!in_register) {
                    gen_code << "\t" << "lea rbx, " << location << std::endl;
                }
                else {
                    gen_code << "\t" << "mov rbx, " << location << std::endl;
                }
            }
            else {
                dest = location;
            }

            address_for_lea = location;
        }
    }

    return destination_information(dest, gen_code.str(), address_for_lea, in_register);
}

bool assign_utilities::requires_copy(DataType t) {
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
