/*

assign_util.cpp
Copyright 2020 Riley Lannon

*/

#include "assign_util.h"

assign_utilities::destination_information::destination_information(
    std::string dest_location,
    std::string fetch_instructions,
    std::string address_for_lea,
    bool in_register,
    bool can_use_lea,
    MoveInstruction instruction_used
) {
    this->dest_location = dest_location;
    this->fetch_instructions = fetch_instructions;
    this->address_for_lea = address_for_lea;
    this->in_register = in_register;
    this->can_use_lea = can_use_lea;
    this->instruction_used = instruction_used;
}

assign_utilities::destination_information assign_utilities::fetch_destination_operand(
    Expression &exp,
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
    bool in_register = false;
    bool can_use_lea = false;
    MoveInstruction instruction_used;

    // generate code based on the expression type
    if (exp.get_expression_type() == IDENTIFIER) {
        // get the symbol information
        auto &lhs = dynamic_cast<Identifier&>(exp);
        auto sym = symbols.find(lhs.getValue());
        auto p = fetch_destination_operand(*sym, symbols, line, r, is_initialization);
        dest = p.dest_location;
        address_for_lea = p.address_for_lea;
        can_use_lea = p.can_use_lea;
        in_register = p.in_register;
        gen_code << p.fetch_instructions;
        instruction_used = p.instruction_used;

        // marks the symbol as initialized
        sym->set_initialized();
    }
    else if (exp.get_expression_type() == UNARY) {
        // get the unary
        auto &lhs = dynamic_cast<Unary&>(exp);
        if (lhs.get_operator() == exp_operator::DEREFERENCE) {
            // ensure the expression has a pointer type; else, indirection is illegal
            auto op_t = expression_util::get_expression_data_type(lhs.get_operand(), symbols, structures, line);
            if (op_t.get_primary() == PTR) {
                auto fetched = fetch_destination_operand(
                    lhs.get_operand(),
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
                can_use_lea = fetched.can_use_lea;
                gen_code << fetched.fetch_instructions;
                in_register = fetched.in_register;
                
                // now, add an instruction to move the previously fetched destination into RBX
                gen_code << "\t" << "mov rbx, " << fetched.dest_location << std::endl;
                instruction_used = MoveInstruction::MOV;
            }
            else {
                throw IllegalIndirectionException(line);
            }
        }
        else {
            throw NonModifiableLValueException(line);
        }
    }
    else if (exp.get_expression_type() == BINARY) {
        auto &lhs = dynamic_cast<Binary&>(exp);
        if (lhs.get_operator() == DOT) {
            dest = "[rbx]";
            gen_code << expression_util::get_exp_address(exp, symbols, structures, r, line).str();
            can_use_lea = false;
            address_for_lea = "rbx";
            instruction_used = MoveInstruction::LEA;
        }
        else {
            throw NonModifiableLValueException(line);
        }
    }
    else if (exp.get_expression_type() == INDEXED) {
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
    
    return destination_information(dest, gen_code.str(), address_for_lea, in_register, can_use_lea, instruction_used);
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
    bool can_use_lea = false;
    MoveInstruction instruction_used;

    auto dt = sym.get_data_type();

    // ensure we can actually assign to it
    if (sym.get_symbol_type() != SymbolType::VARIABLE) {
        throw InvalidSymbolException(line);
    }
    else if (dt.get_qualities().is_const() && !is_initialization) {
        throw ConstAssignmentException(line);
    }
    else if (dt.get_qualities().is_final() && !is_initialization && sym.was_initialized()) {    // functions require parameters to be marked as initialized, so include the initialization param here to avoid errors in the call
        throw FinalAssignmentException(line);
    }
    else {
        if (dt.get_qualities().is_static()) {
            dest = "[rbx]";
            address_for_lea = "[" + sym.get_name() + "]";
            can_use_lea = true;
            gen_code << "\t" << "lea rbx, [" << sym.get_name() << "]" << std::endl;
            instruction_used = MoveInstruction::LEA;
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
                instruction_used = MoveInstruction::MOV;
            }
            else if (requires_copy(sym.get_data_type())) {
                // if we don't have a string but we do require a copy, use lea
                // but if that value is in a register, just use mov
                dest = "[rbx]";
                if (in_register) {
                    gen_code << "\t" << "mov rbx, " << location << std::endl;
                    instruction_used = MoveInstruction::MOV;
                }
                else {
                    gen_code << "\t" << "lea rbx, " << location << std::endl;
                    instruction_used = MoveInstruction::LEA;
                }
            }
            else {
                dest = location;
            }

            address_for_lea = location;
            can_use_lea = !in_register;
        }
    }

    return destination_information(dest, gen_code.str(), address_for_lea, in_register, can_use_lea, instruction_used);
}

bool assign_utilities::requires_copy(DataType t) {
    /*

    requires_copy
    Returns whether the given type requires a copy routine or can be assigned directly

    */

    return (
        t.get_primary() == STRING ||
        t.get_primary() == ARRAY ||
        t.get_primary() == TUPLE ||
        t.get_primary() == STRUCT
    );
}

bool assign_utilities::is_valid_move_expression(std::shared_ptr<Expression> exp) {
    /*

    is_valid_move_expression
    Determines whether the given expression may be used in a move statement

    Move expressions must be modifiable-lvalues, meaning they can be:
        * Identifiers
        * Binary expressions using the dot operator
        * Unary expressions using the dereference operator
        * Indexed expressions
    They can't be const, though they may be final (though assignment to initialized final data is still illegal)

    */

    bool is_valid;

    if (
        exp->get_expression_type() == LITERAL ||
        exp->get_expression_type() == CALL_EXP
    ) {
        is_valid = false;
    }
    else if (exp->get_expression_type() == BINARY) {
        auto b = dynamic_cast<Binary*>(exp.get());
        if (b->get_operator() == DOT) {
            is_valid = true;
        }
        else {
            is_valid = false;
        }
    }
    else if (exp->get_expression_type() == UNARY) {
        auto u = dynamic_cast<Unary*>(exp.get());
        if (u->get_operator() == DEREFERENCE) {
            is_valid = true;
        }
        else {
            is_valid = false;
        }
    }
    else {
        is_valid = true;
    }

    return is_valid;
}
