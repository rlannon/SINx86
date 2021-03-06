/*

SIN Toolchain (x86 target)
assign.cpp

Implementation of the assignment functions for the compiler

*/

#include "compiler.h"
#include "compile_util/assign_util.h"
#include "compile_util/function_util.h"

// todo: overhaul assignment

std::stringstream compiler::handle_assignment(const Assignment &a) {
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
    auto lhs_type = expression_util::get_expression_data_type(a.get_lvalue(), this->symbols, this->structs, a.get_line_number());
    auto rhs_type = expression_util::get_expression_data_type(a.get_rvalue(), this->symbols, this->structs, a.get_line_number());

    std::stringstream handle_ss;

    // if we have an indexed expression as the lvalue, we need a special case (for code generation)
    if (a.get_lvalue().get_expression_type() == INDEXED) {
        // make sure that the type is actually indexable/subscriptable
        auto &idx = static_cast<const Indexed&>(a.get_lvalue());
        if (!is_subscriptable(
                expression_util::get_expression_data_type(
                    idx.get_to_index(), 
                    this->symbols, 
                    this->structs,
                    a.get_line_number()
                ).get_primary()
            )
        ) {
            throw TypeNotSubscriptableException(a.get_line_number());
        }

        // overwrite p.second with the actual destination fetch code
        std::stringstream overwrite;

        // if RAX is used to fetch the RHS, we need to preserve it on the stack before determining the index
        // note that floating point types are exempt here since XMM registers will be used
        bool must_push = rhs_type.get_primary() != FLOAT;
        if (must_push)
            overwrite << "\t" << "push rax" << std::endl;
        
        overwrite << this->get_exp_address(a.get_lvalue(), RBX, a.get_line_number()).str();
        
        if (must_push)
            overwrite << "\t" << "pop rax" << std::endl;
        
        p.fetch_instructions = overwrite.str();
    }

    handle_ss << this->assign(lhs_type, rhs_type, p, a.get_rvalue(), a.get_line_number()).str();

    return handle_ss;
}

std::stringstream compiler::handle_alloc_init(const symbol &sym, const Expression& rvalue, unsigned int line) {
    /*

    handle_alloc_init
    Handles initialization in an 'alloc' statement

    Dispatches to 'assign'

    */

    auto p = assign_utilities::fetch_destination_operand(sym, this->symbols, line, RBX, true);
    auto rhs_type = expression_util::get_expression_data_type(rvalue, this->symbols, this->structs, line);

    reg src_reg = sym.get_data_type().get_primary() == FLOAT ? XMM0 : RAX;

    // we need to have a special case for ref<T> initialization
    /*if (sym.get_data_type().get_primary() == REFERENCE) {
        // wrap the rvalue in a unary address-of expression
        // this will be fine since we will be comparing against the subtype but evaluating a pointer
        std::unique_ptr<Unary> u = std::make_unique<Unary>(
            rvalue,
            exp_operator::ADDRESS
        );
        return this->assign(sym.get_data_type(), rhs_type, p, *u, line, true);
    }
    else {*/
    // todo: we can utilize the copy construction method for alloc-init when used with dynamic types
        return this->assign(sym.get_data_type(), rhs_type, p, rvalue, line, true);
    //}
}

std::stringstream compiler::assign(
    const DataType& lhs_type,
    const DataType &rhs_type,
    assign_utilities::destination_information dest,
    const Expression &rvalue,
    unsigned int line,
    bool is_alloc_init
) {
    /*

    assign
    Generates code for the actual assignment

    */
    
    std::stringstream handle_assign;
    size_t count = 0;
    
    // get the source register
    reg src_reg = rhs_type.get_primary() == FLOAT ? XMM0 : RAX;

    if (lhs_type.is_compatible(rhs_type)) {
        // first, call sre_free on the lhs if we have a managed pointer (and it's not alloc-init)
        if (lhs_type.get_primary() == PTR && lhs_type.get_qualities().is_managed() && !is_alloc_init) {
            handle_assign << push_used_registers(this->reg_stack.peek(), true).str();
            handle_assign << "\t" << "mov rdi, " << dest.dest_location << std::endl;
            handle_assign << function_util::call_sre_function(magic_numbers::SRE_FREE);
            handle_assign << pop_used_registers(this->reg_stack.peek(), true).str();
        }

        // evaluate the rvalue, then the destination (lvalue)
        auto handle_p = this->evaluate_expression(rvalue, line, &lhs_type); // ensure we give evaluate_expression the lhs type as a hint
        handle_assign << handle_p.first;
        bool do_free = handle_p.second > 0;
        
        handle_assign << dest.fetch_instructions;

        // get the appropriate variant of RAX based on the width of the type to which we are assigning
        std::string src = register_usage::get_register_name(src_reg, lhs_type);

        // make the assignment
        if (lhs_type.get_primary() == TUPLE) {
            handle_assign << push_used_registers(this->reg_stack.peek(), true).str();

            // set up our registers/arguments
            handle_assign << "\t" << "mov rsi, rax" << std::endl;
            handle_assign << "\t" << "mov rdi, rbx" << std::endl;

            // copy byte for byte
            // todo: ensure array evaluation utilizes type hints so that the data is the appropriate number of bytes
            handle_assign << "\t" << "mov rcx, " << lhs_type.get_width() << std::endl;
            handle_assign << "\t" << "rep movsb" << std::endl;

            handle_assign << pop_used_registers(this->reg_stack.peek(), true).str();
        }
        else if (assign_utilities::requires_copy(lhs_type)) {
            handle_assign << push_used_registers(this->reg_stack.peek(), true).str();

            // set up our registers/arguments
            handle_assign << "\t" << "mov rsi, rax" << std::endl;
            std::string destination_register_operand;

            if (dest.instruction_used == assign_utilities::MoveInstruction::LEA) {
                if (lhs_type.get_primary() == STRING || lhs_type.get_qualities().is_dynamic()) {
                    destination_register_operand = "[rbx]";
                }
                else {
                    destination_register_operand = "rbx";
                }
            }
            else {
                destination_register_operand = "rbx";
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

                proc_name = "sinl_string_copy";
            }
            // todo: other copy types

            // call the function
            handle_assign << function_util::call_sincall_subroutine(proc_name);

            // now, if we had a string, we need to move the returned address into where the string is located
            if (lhs_type.get_primary() == STRING) {
                handle_assign << "\t" << assign_instruction << std::endl;
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
            
            // if the returned value was a reference, we just performed a reference copy here
            if (do_free)
                do_free = false;
        }

        // now, call sre_add_ref on the lhs if we have a managed pointer OR if we have a reference and alloc-init
        if (
            (lhs_type.get_primary() == PTR && lhs_type.get_qualities().is_managed()) ||
            (lhs_type.get_primary() == REFERENCE && is_alloc_init)
        ) {
            handle_assign << push_used_registers(this->reg_stack.peek(), true).str();
            handle_assign << "\t" << "mov rdi, " << dest.dest_location << std::endl;
            handle_assign << function_util::call_sre_function(magic_numbers::SRE_ADD_REF);
            handle_assign << pop_used_registers(this->reg_stack.peek(), true).str();
        }

        // if the assignment was done via a temporary reference, we need to free it
        if (do_free) {
            handle_assign << "\t" << "pop rax" << std::endl;
            handle_assign << push_used_registers(this->reg_stack.peek(), true).str();
            handle_assign << "\t" << "mov rdi, rax" << std::endl;
            handle_assign << function_util::call_sre_function(magic_numbers::SRE_FREE);
            handle_assign << pop_used_registers(this->reg_stack.peek(), true).str();
        }
    }
    else {
        throw TypeException(line);
    }

    return handle_assign;
}
