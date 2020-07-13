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
        
        p.fetch_instructions = overwrite.str();
    }
    
    // we need to adjust our reference counts for pointers
    if (lhs_type.get_primary() == PTR) {
        //
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

    // we need to have a special case for ref<T> initialization
    if (sym.get_data_type().get_primary() == REFERENCE) {
        // wrap the rvalue in a unary address-of expression
        // this will be fine since we will be comparing against the subtype but evaluating a pointer
        rvalue = std::make_shared<Unary>(
            rvalue,
            exp_operator::ADDRESS
        );
    }
    
    return this->assign(sym.get_data_type(), rhs_type, p, rvalue, line, true);
}

std::stringstream compiler::assign(
    DataType lhs_type,
    DataType &rhs_type,
    assign_utilities::destination_information dest,
    std::shared_ptr<Expression> rvalue,
    unsigned int line,
    bool is_alloc_init
) {
    /*

    assign
    Generates code for the actual assignment

    */
    
    std::stringstream handle_assign;
    
    // get the source register
    reg src_reg = rhs_type.get_primary() == FLOAT ? XMM0 : RAX;

    if (lhs_type.is_compatible(rhs_type)) {
        // first, call sre_free on the lhs if we have a pointer
        if (lhs_type.get_primary() == PTR) {
            handle_assign << push_used_registers(this->reg_stack.peek(), true).str();
            handle_assign << "\t" << "mov rdi, " << dest.dest_location << std::endl;
            handle_assign << "\t" << "call sre_free" << std::endl;
            handle_assign << pop_used_registers(this->reg_stack.peek(), true).str();
        }

        // evaluate the rvalue, then the destination (lvalue)
        handle_assign << this->evaluate_expression(rvalue, line).str();
        handle_assign << dest.fetch_instructions;

        // get the appropriate variant of RAX based on the width of the type to which we are assigning
        std::string src = register_usage::get_register_name(src_reg, lhs_type);

        // make the assignment
        if (assign_utilities::requires_copy(lhs_type)) {
            handle_assign << push_used_registers(this->reg_stack.peek(), true).str();

            // set up our registers/arguments
            handle_assign << "\t" << "mov rsi, rax" << std::endl;
            handle_assign << "\t" << "mov rdi, rbx" << std::endl;

            std::string proc_name;

            // if we have an array
            if (lhs_type.get_primary() == ARRAY) {
                handle_assign << "\t" << "mov ecx, " << lhs_type.get_full_subtype()->get_width() << std::endl;
                proc_name = "sinl_array_copy";
            }
            else {
                handle_assign << "\t" << "lea r15, " << dest.address_for_lea << std::endl;
                proc_name = "sinl_string_copy";
            }
            // todo: other copy types

            // set up the stack frame; call the function
            handle_assign << "\t" << "pushfq" << std::endl;
            handle_assign << "\t" << "push rbp" << std::endl;
            handle_assign << "\t" << "mov rbp, rsp" << std::endl;

            handle_assign << "\t" << "call " << proc_name << std::endl;

            // restore our old stack frame             
            handle_assign << "\t" << "mov rsp, rbp" << std::endl;
            handle_assign << "\t" << "pop rbp" << std::endl;
            handle_assign << "\t" << "popfq" << std::endl;

            // now, if we had a string, we need to move the returned address into where the string is located
            if (lhs_type.get_primary() == STRING) {
                handle_assign << "\t" << "mov [r15], rax" << std::endl;
            }

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

            handle_assign << "\t" << instruction << " " << dest.dest_location << ", " << src << std::endl; 
        }

        // now, call sre_add_ref on the lhs if we have a pointer OR if we have a reference and alloc-init
        if (
            (lhs_type.get_primary() == PTR) ||
            (lhs_type.get_primary() == REFERENCE && is_alloc_init)
        ) {
            handle_assign << push_used_registers(this->reg_stack.peek(), true).str();
            handle_assign << "\t" << "mov rdi, " << dest.dest_location << std::endl;
            handle_assign << "\t" << "call sre_add_ref" << std::endl;
            handle_assign << pop_used_registers(this->reg_stack.peek(), true).str();
        }
    }
    else {
        throw TypeException(line);
    }

    return handle_assign;
}
