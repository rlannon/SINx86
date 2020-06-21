/*

SIN Toolchain (x86 target)
pointers.cpp
Copyright 2020 Riley Lannon

Implements various functionality for pointers within the compiler

*/

#include "compiler.h"

std::stringstream compiler::get_address(Unary &u, unsigned int line) {
    /*

    Gets the address of the expression contained within the unary

    */

    std::stringstream addr_ss;
			
    // how we generate code for this depends on the type
    if (u.get_operand()->get_expression_type() == BINARY) {
        // if we have a binary expression, it *must* be the dot operator; if so, just return what's in RBX after we evaluate it
        Binary* target = dynamic_cast<Binary*>(u.get_operand().get());
        if (target->get_operator() != DOT) {
            throw CompilerException("Illegal binary operand in address-of expression", compiler_errors::ILLEGAL_ADDRESS_OF_ARGUMENT, line);
        }

        addr_ss << this->evaluate_binary(*target, line).str();
        addr_ss << "mov rax, rbx" << std::endl;
        this->reg_stack.peek().clear(RBX);  // now we can use RBX again
    } else if (u.get_operand()->get_expression_type() == LVALUE) {
        LValue* target = dynamic_cast<LValue*>(u.get_operand().get());
        // look up the symbol; obtain the address based on its memory location
        std::shared_ptr<symbol> s = this->lookup(target->getValue(), line);
        if (s->get_data_type().get_qualities().is_static()) {
            addr_ss << "\t" << "mov rax, " << s->get_name() << std::endl;
        } 
        else {
            // first, get the stack address
            addr_ss << "\t" << "mov rax, rbp" << std::endl;
            addr_ss << "\t" << "sub rax, " << s->get_offset() << std::endl;

            // if the variable is dynamic or a string, dereference RAX
            if (s->get_data_type().get_qualities().is_dynamic() || s->get_data_type().get_primary() == STRING) {
                addr_ss << "\t" << "mov rax, [rax]" << std::endl;
            }
        }
    } else {
        throw CompilerException("Illegal address-of argument", compiler_errors::ILLEGAL_ADDRESS_OF_ARGUMENT, line);
    }

    return addr_ss;
}
