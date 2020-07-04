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

    Dispatches to 'assign'

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

    std::stringstream handle_ss;

    // if we have an indexed expression as the lvalue, we need a special case (for code generation)
    if (a.get_lvalue()->get_expression_type() == INDEXED) {
        // overwrite p.second with the actual destination fetch code
        std::stringstream overwrite;

        // if RAX is used to fetch the RHS, we need to preserve it on the stack before determining the index
        bool must_push = rhs_type.get_primary() != FLOAT;
        if (must_push)
            overwrite << "\t" << "push rax" << std::endl;
        
        overwrite << this->get_exp_address(a.get_lvalue(), RBX, a.get_line_number()).str();
        
        if (must_push)
            overwrite << "\t" << "pop rax" << std::endl;
        
        p.second = overwrite.str();
    }
    
    handle_ss << this->assign(lhs_type, rhs_type, p, a.get_rvalue(), a.get_line_number()).str();

    return handle_ss;
}

std::stringstream compiler::handle_alloc_init(symbol &sym, std::shared_ptr<Expression> rvalue, unsigned int line) {
    /*

    handle_alloc_init
    Handles initialization in an 'alloc' statement

    Dispatches to 'assign'

    */

    auto p = assign_utilities::fetch_destination_operand(sym, this->symbols, line, RBX, true);
    auto rhs_type = get_expression_data_type(rvalue, this->symbols, this->structs, line);

    reg src_reg = sym.get_data_type().get_primary() == FLOAT ? XMM0 : RAX;

    return this->assign(sym.get_data_type(), rhs_type, p, rvalue, line);
}

std::stringstream compiler::assign(DataType lhs_type, DataType &rhs_type, std::pair<std::string, std::string> dest, std::shared_ptr<Expression> rvalue, unsigned int line) {
    /*

    assign
    Generates code for the actual assignment

    */
    
    std::stringstream handle_assign;
    
    // get the source register
    reg src_reg = rhs_type.get_primary() == FLOAT ? XMM0 : RAX;

    if (lhs_type.is_compatible(rhs_type)) {
        // evaluate the rvalue, then the destination (lvalue)
        handle_assign << this->evaluate_expression(rvalue, line).str();
        handle_assign << dest.second;

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

            handle_assign << "\t" << instruction << " " << dest.first << ", " << src << std::endl; 
        }
    }
    else {
        throw TypeException(line);
    }

    return handle_assign;
}
