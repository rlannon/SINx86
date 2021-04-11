/*

assign_util.cpp
Copyright 2020 Riley Lannon

*/

#include "assign_util.h"
#include "function_util.h"

assign_utilities::destination_information::destination_information(
    const std::string& dest_location,
    const std::string& fetch_instructions,
    const std::string& address_for_lea,
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
    const Expression &exp,
    symbol_table &symbols,
    struct_table &structures,
    const std::string& scope_name,
    unsigned int scope_level,
    unsigned int line,
    reg r,
    bool is_initialization
) {
    /*

    fetch_destination_operand
    Fetches the destination for the given expression
    
    This function returns both the location and the code to fetch it.

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
        auto &lhs = static_cast<const Identifier&>(exp);
        auto &sym = symbols.find(lhs.getValue());
        auto p = fetch_destination_operand(sym, symbols, line, r, is_initialization);
        dest = p.dest_location;
        address_for_lea = p.address_for_lea;
        can_use_lea = p.can_use_lea;
        in_register = p.in_register;
        gen_code << p.fetch_instructions;
        instruction_used = p.instruction_used;

        // marks the symbol as initialized
        sym.set_initialized();
    }
    else if (exp.get_expression_type() == UNARY) {
        // get the unary
        auto &lhs = static_cast<const Unary&>(exp);
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
        auto &lhs = static_cast<const Binary&>(exp);
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
    const symbol &sym,
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
    MoveInstruction instruction_used = MoveInstruction::MOV;

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

bool assign_utilities::requires_copy(const DataType& t) {
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

bool assign_utilities::is_valid_move_expression(const Expression &exp) {
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
        exp.get_expression_type() == LITERAL ||
        exp.get_expression_type() == CALL_EXP
    ) {
        is_valid = false;
    }
    else if (exp.get_expression_type() == BINARY) {
        auto &b = static_cast<const Binary&>(exp);
        if (b.get_operator() == DOT) {
            is_valid = true;
        }
        else {
            is_valid = false;
        }
    }
    else if (exp.get_expression_type() == UNARY) {
        auto &u = static_cast<const Unary&>(exp);
        if (u.get_operator() == DEREFERENCE) {
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

std::string assign_utilities::do_assign(
    const reg src_reg,
    const DataType& lhs_type,
    const destination_information& dest,
    register_usage& context, 
    const unsigned int line,
    bool& do_free
) {
    /*

    do_assign
    Generates code for an assignment from 'src_reg' to 'dest'

    */

    std::stringstream handle_assign;

    std::string src = register_usage::get_register_name(src_reg, lhs_type);

    // make the assignment
    if (lhs_type.get_primary() == TUPLE) {
        handle_assign << push_used_registers(context, true).str();

        // set up our registers/arguments
        handle_assign << "\t" << "mov rsi, rax" << std::endl;
        handle_assign << "\t" << "mov rdi, rbx" << std::endl;

        // copy byte for byte
        // todo: ensure array evaluation utilizes type hints so that the data is the appropriate number of bytes
        handle_assign << "\t" << "mov rcx, " << lhs_type.get_width() << std::endl;
        handle_assign << "\t" << "rep movsb" << std::endl;

        handle_assign << pop_used_registers(context, true).str();
    }
    else if (assign_utilities::requires_copy(lhs_type)) {
        handle_assign << push_used_registers(context, true).str();

        // set up our registers/arguments
        handle_assign << "\t" << "mov rsi, rax" << std::endl;
        std::string destination_register_operand = "rbx";
        if (
            (dest.instruction_used == assign_utilities::MoveInstruction::LEA) &&
            (lhs_type.get_primary() == STRING || lhs_type.get_qualities().is_dynamic())
        ) {
            destination_register_operand = "[rbx]";
        }
        handle_assign << "\t" << "mov rdi, " << destination_register_operand << std::endl;
        
        std::string proc_name;
        std::string assign_instruction; // if we are storing the reference in a register, we will need a different assign instruction

        // if we have an array, we don't need to worry about the address changing (they are never resized by array_copy)
        if (lhs_type.get_primary() == ARRAY) {
            handle_assign << "\t" << "mov ecx, " << lhs_type.get_subtype().get_width() << std::endl;
            proc_name = "sinl_array_copy";
        }
        // strings are different; they are automatically resized and so string_copy will return an address
        else {
            if (dest.in_register) {
                assign_instruction = "mov " + dest.address_for_lea + ", rax";
            }
            else {
                // todo: verify this works generally
                // if we don't have an address (e.g., it's a binary), then we can use mov with the dest
                if (dest.can_use_lea) {
                    handle_assign << "\t" << "lea r15, " << dest.address_for_lea << std::endl;
                }
                else {
                    handle_assign << "\t" << "mov r15, " << dest.address_for_lea << std::endl;  // still uses 'address_for_lea'
                }
                assign_instruction = "mov [r15], rax";
            }

            proc_name = "sinl_string_copy"; // todo: why is this being done with a struct? this should just be a memcpy
        }
        // todo: other copy types

        // call the function
        handle_assign << function_util::call_sincall_subroutine(proc_name);

        // now, if we had a string, we need to move the returned address into where the string is located
        if (lhs_type.get_primary() == STRING) {
            handle_assign << "\t" << assign_instruction << std::endl;
        }

        handle_assign << pop_used_registers(context, true).str();
    }
    else {
        // move 'src' into the 'location' (a register or memory address)
        std::string instruction;
        if (lhs_type.get_primary() == FLOAT) {
            instruction = lhs_type.get_width() == sin_widths::DOUBLE_WIDTH ? "movsd" : "movss";
        }
        else {
            instruction = "mov";
        }

        handle_assign << "\t" << instruction << " " << dest.dest_location << ", " << src << std::endl;
        
        // if the returned value was a reference, we just performed a reference copy here
        if (do_free)
            do_free = false;
    }
    
    return handle_assign.str();
}
