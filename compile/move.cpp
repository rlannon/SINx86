/*

SIN Toolchain (x86 target)
move.cpp
Copyright 2020 Riley Lannon

Contains the function for handling move assignments

*/

#include "compiler.h"



std::stringstream compiler::handle_move(Movement &m) {
    /*

    handle_move
    Handles a move assignment

    */

    std::stringstream move_ss;

    // we need to get the lvalue and rvalues and validate their expression types
    if (
        assign_utilities::is_valid_move_expression(m.get_lvalue()) &&
        assign_utilities::is_valid_move_expression(m.get_rvalue())
    ) {
        auto lvalue_type = expression_util::get_expression_data_type(
            m.get_lvalue(),
            this->symbols,
            this->structs,
            m.get_line_number()
        );
        auto rvalue_type = expression_util::get_expression_data_type(
            m.get_rvalue(),
            this->symbols,
            this->structs,
            m.get_line_number()
        );

        // todo: ensure this works properly for all valid combinations of reference types

        // if the type is a reference type, we can move; otherwise, form an assignment
        if (lvalue_type.is_reference_type()) {
            // we can't move to references though -- the reference may never be updated
            if (lvalue_type.get_primary() == REFERENCE) {
                throw CompilerException(
                    "Move assignment not allowed with references; the reference is final",
                    compiler_errors::MOVE_TO_REFERENCE_ERROR,
                    m.get_line_number()
                );
            }

            // fetch destination information
            auto dest = assign_utilities::fetch_destination_operand(
                m.get_lvalue(),
                this->symbols,
                this->structs,
                this->current_scope_name,
                this->current_scope_level,
                m.get_line_number()
            );
            // todo: this doesn't actually get the proper address in RBX
            // it uses a mov when an lea would be more appropriate

            move_ss << this->move(
                lvalue_type,
                rvalue_type,
                dest,
                m.get_rvalue(),
                m.get_line_number()
            ).str();
        }
        else {
            move_ss = this->handle_assignment(m);   // since Movement is a child of Assignment, we can just pass in m
        }
    }
    else {
        throw CompilerException(
            "Illegal expression in move assignment; expression must be a modifiable-lvalue",
            compiler_errors::ILLEGAL_MOVE_ASSIGNMENT_EXPRESSION,
            m.get_line_number()
        );
    }

    return move_ss;
}

std::stringstream compiler::move(
    DataType &lvalue_type,
    DataType &rvalue_type,
    assign_utilities::destination_information dest,
    std::shared_ptr<Expression> rvalue,
    unsigned int line
) {
    /*

    move
    Moves data between locations by updating references.

    Note this is not called if the move assignment is like
        alloc int a: 10;
        alloc int b;
        move a -> b;
    as this will just perform a copy assignment.

    */

    std::stringstream move_ss;

    // if the types are compatible, copy the reference
    if (lvalue_type.is_compatible(rvalue_type)) {
        // evaluate the rvalue
        auto handle_p = this->evaluate_expression(rvalue, line);
        move_ss << handle_p.first;
        move_ss << "\t" << "lea rbx, " << dest.address_for_lea << std::endl;
        this->reg_stack.peek().set(RBX);

        // first, free the value at the reference
        move_ss << push_used_registers(this->reg_stack.peek(), false).str();
        move_ss << "\t" << "mov rdi, [rbx]" << std::endl;
        move_ss << call_sre_function(magic_numbers::SRE_FREE);
        move_ss << pop_used_registers(this->reg_stack.peek(), false).str();

        move_ss << "\t" << "mov [rbx], rax" << std::endl;
        this->reg_stack.peek().clear(RBX);

        // now add a reference to the new value
        move_ss << push_used_registers(this->reg_stack.peek(), true).str();
        move_ss << "\t" << "mov rdi, rax" << std::endl;
        move_ss << call_sre_function(magic_numbers::SRE_ADD_REF);
        move_ss << pop_used_registers(this->reg_stack.peek(), true).str();
    }
    else {
        throw TypeException(line);
    }

    return move_ss;
}
