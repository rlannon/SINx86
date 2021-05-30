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
    if (sym.get_data_type().get_primary() == REFERENCE) {
        // wrap the rvalue in a unary address-of expression
        // this will be fine since we will be comparing against the subtype but evaluating a pointer
        std::unique_ptr<Unary> u = std::make_unique<Unary>(
            rvalue.clone(),
            exp_operator::ADDRESS
        );
        return this->assign(sym.get_data_type(), rhs_type, p, *u, line, true);
    }
    else {
    // todo: we can utilize the copy construction method for alloc-init when used with dynamic types
        return this->assign(sym.get_data_type(), rhs_type, p, rvalue, line, true);
    }
}

std::stringstream compiler::assign(
    const DataType& lhs_type,
    const DataType &rhs_type,
    const assign_utilities::destination_information& dest,
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
        auto handle_p = this->evaluate_expression(rvalue, line, &lhs_type, &dest); // ensure we give evaluate_expression the lhs type as a hint
        handle_assign << handle_p.first;
        bool do_free = handle_p.second > 0;

        // if we have alloc_init and a construction expression, don't call do_assign
        if ( !(is_alloc_init && rvalue.get_expression_type() == CONSTRUCTION_EXP))
        {   
            handle_assign << dest.fetch_instructions;

            handle_assign << assign_utilities::do_assign(
                src_reg,
                lhs_type,
                dest,
                this->reg_stack.peek(),
                line,
                do_free,
                structs
            );
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
