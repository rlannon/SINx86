/*

SIN Toolchain (x86 target)
pointers.cpp
Copyright 2020 Riley Lannon

Implements various functionality for pointers within the compiler

*/

#include "compiler.h"

std::stringstream compiler::get_exp_address(const Expression &exp, reg r, unsigned int line) {
    /*

    get_exp_address
    Gets the address of an expression, using our utility to help

    */

    // first, use the utility function
    std::stringstream addr_ss = expression_util::get_exp_address(exp, this->symbols, this->structs, r, line);

    // now, make any adjustments we need to
    if (exp.get_expression_type() == INDEXED) {
        // we need to adjust the value of 'r' by the index value
        auto &i = static_cast<const Indexed&>(exp);
        DataType idx_type = expression_util::get_expression_data_type(i.get_to_index(), this->symbols, this->structs, line);

        // if RCX is in use, preserve it -- we are using it for 'mul'
        if (this->reg_stack.peek().is_in_use(RCX)) {
            addr_ss << "\t" << "push rcx" << std::endl;
        }

        // store 'r' in an available register if it's in RAX or RBX
        std::string r_name = register_usage::get_register_name(r);
        bool pushed = false;
        reg temp = NO_REGISTER;
        if (r == RBX) {
            temp = this->reg_stack.peek().get_available_register(PTR);
            if (temp == NO_REGISTER) {
                pushed = true;
                addr_ss << "\t" << "push " << r_name << std::endl;
            }
            else {
                this->reg_stack.peek().set(temp);
                addr_ss << "\t" << "mov " << register_usage::get_register_name(temp) << ", " << r_name << std::endl;
            }
        }
        else {
            this->reg_stack.peek().set(r);
        }

        // the index value will be in eax and the array length will be in [rax]

        // evaluate the index value and multiply by the type width
        auto exp_address_p = this->evaluate_expression(i.get_index_value(), line);
        addr_ss << exp_address_p.first;
        
        // now, restore our values if we needed to make any adjustments
        if (r == RBX) {
            if (pushed) {
                addr_ss << "\t" << "pop " << r_name << std::endl;
            }
            else {
                addr_ss << "\t" << "mov " << r_name << ", " << register_usage::get_register_name(temp) << std::endl;
                this->reg_stack.peek().clear(temp);
            }
        }

        // ensure we are within the bounds of the array
        addr_ss << "\t" << "cmp [rbx], eax" << std::endl;
        addr_ss << "\t" << "jg .sinl_rtbounds_" << this->rtbounds_num << std::endl;

        // if we were out of bounds, call the appropriate function
        addr_ss << "\t" << "call " << magic_numbers::SINL_RTE_OUT_OF_BOUNDS << std::endl;
        
        addr_ss << ".sinl_rtbounds_" << this->rtbounds_num << ":" << std::endl;

        // todo: check to see if rdx is in use so the value can be preserved
        addr_ss << "\t" << "mov edx, 0" << std::endl;
        addr_ss << "\t" << "mov ecx, " << idx_type.get_subtype().get_width() << std::endl;
        addr_ss << "\t" << "mul ecx" << std::endl;

        if (this->reg_stack.peek().is_in_use(RCX)) {
            addr_ss << "\t" << "pop rcx" << std::endl;
        }

        // finally, adjust the values
        addr_ss << "\t" << "add rax, " << sin_widths::INT_WIDTH << std::endl;
        addr_ss << "\t" << "add " << r_name << ", rax" << std::endl;

        // increment our scope block number
        this->rtbounds_num += 1;
    }

    return addr_ss;
}

std::stringstream compiler::get_address_of(const Unary &u, reg r, unsigned int line) {
    /*

    Gets the address of the expression contained within the unary

    */

    std::stringstream addr_ss;
			
    // how we generate code for this depends on the type
    if (u.get_operand().get_expression_type() == BINARY) {
        // if we have a binary expression, it *must* be the dot operator; if so, just return what's in RBX after we evaluate it
        auto &target = static_cast<const Binary&>(u.get_operand());
        if (target.get_operator() != DOT) {
            throw CompilerException("Illegal binary operand in address-of expression", compiler_errors::ILLEGAL_ADDRESS_OF_ARGUMENT, line);
        }

        auto bin_p = this->evaluate_binary(target, line);
        addr_ss << bin_p.first;
        addr_ss << "mov " << register_usage::get_register_name(r) << ", rbx" << std::endl;
        this->reg_stack.peek().clear(r);  // now we can use RBX again
    }
    else if (u.get_operand().get_expression_type() == IDENTIFIER) {
        auto &target = static_cast<const Identifier&>(u.get_operand());
        
        // look up the symbol; obtain the address based on its memory location
        symbol *s = this->lookup(target.getValue(), line);
        addr_ss << get_address(*s, r);
    }
    else if (u.get_operand().get_expression_type() == INDEXED) {

    }
    else {
        throw CompilerException("Illegal address-of argument", compiler_errors::ILLEGAL_ADDRESS_OF_ARGUMENT, line);
    }

    return addr_ss;
}
