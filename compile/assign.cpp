/*

SIN Toolchain (x86 target)
assign.cpp

Implementation of the assignment functions for the compiler

*/

#include "compiler.h"
#include "compile_util/assign_util.h"

// todo: overhaul assignment

std::stringstream compiler::handle_assignment(Assignment &a) {
    /*

    handle_assignment
    Handles assignments from assignment statements

    */

    std::stringstream handle_assign;

    // fetch the destination operand
    auto p = assign_utilities::fetch_destination_operand(
        a.get_lvalue(),
        this->symbols,
        this->structs,
        this->current_scope_name,
        this->current_scope_level,
        a.get_line_number()
    );

    // get the data type of each expression
    auto lhs_type = get_expression_data_type(a.get_lvalue(), this->symbols, this->structs, a.get_line_number());
    auto rhs_type = get_expression_data_type(a.get_rvalue(), this->symbols, this->structs, a.get_line_number());

    // get the source register
    reg src_reg = rhs_type.get_primary() == FLOAT ? XMM0 : RAX;

    if (lhs_type.is_compatible(rhs_type)) {
        // evaluate the rvalue, then the destination (lvalue)
        handle_assign << this->evaluate_expression(a.get_rvalue(), a.get_line_number()).str();
        handle_assign << p.second;

        // get the appropriate variant of RAX based on the width of the type to which we are assigning
        std::string src = register_usage::get_register_name(src_reg, lhs_type);
        
        // make the assignment
        if (assign_utilities::requires_copy(lhs_type)) {
            handle_assign << push_used_registers(this->reg_stack.peek(), true).str();

            handle_assign << "\t" << "mov rsi, rax" << std::endl;
            handle_assign << "\t" << "mov rdi, rbx" << std::endl;

            handle_assign << pop_used_registers(this->reg_stack.peek(), true).str();
        }
        else {
            // move 'src' into 'p.first'
            std::string instruction;
            if (lhs_type.get_primary() == FLOAT) {
                instruction = lhs_type.get_width() == sin_widths::DOUBLE_WIDTH ? "movsd" : "movss";
            }
            else {
                instruction = "mov";
            }

            handle_assign << "\t" << instruction << " " << p.first << ", " << src << std::endl; 
        }
    }
    else {
        throw TypeException(a.get_line_number());
    }

    return handle_assign;
}

std::stringstream compiler::handle_alloc_init(symbol &sym, std::shared_ptr<Expression> rvalue, unsigned int line) {
    /*

    handle_alloc_init
    Handles initialization in an 'alloc' statement

    */

    std::stringstream init_ss;

    auto p = assign_utilities::fetch_destination_operand(sym, this->symbols, line, RBX, true);
    auto rhs_type = get_expression_data_type(rvalue, this->symbols, this->structs, line);

    // ensure the types are compatible before proceeding
    if (sym.get_data_type().is_compatible(rhs_type)) {
        // todo: initialize
    }
    else {
        throw TypeException(line);
    }

    return init_ss;
}

