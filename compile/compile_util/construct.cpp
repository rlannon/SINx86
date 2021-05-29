/*

SIN toolchain (x86 target)
construct.cpp
Copyright 2021 Riley Lannon

Implementation of construction-related functions.

*/

#include "construct.h"
#include "assign_util.h"
#include "../../util/CompilerErrorCodes.h"

#include <sstream>

std::string construct_util::default_construct(const symbol& sym, symbol_table& symbols, struct_table& structs, register_usage& context, const unsigned int line)
{
    /*

    default_construct
    Handles default construction for the given symbol

    */

    if (sym.get_data_type().get_primary() == REFERENCE || sym.get_data_type().get_qualities().is_const())
    {
        throw CompilerException(
            "ref<T> and const-qualified data cannot be default-constructed",
            compiler_errors::ALLOC_INIT_REQUIRED,
            line
        );
    }
    
    std::stringstream construct_ss;

    auto p = assign_utilities::fetch_destination_operand(sym, symbols, line, RBX, true);
    construct_ss << p.fetch_instructions;

    reg to_use = sym.get_data_type().get_primary() == FLOAT ? XMM0 : RAX;
    auto reg_name = register_usage::get_register_name(to_use);
    construct_ss << "\t" << "mov " << reg_name << ", 0" << std::endl;
    bool do_free = false;   // required for the utility

    construct_ss << assign_utilities::do_assign(
        to_use,
        sym.get_data_type(),
        p,
        context,
        line,
        do_free,
        structs
    );

    return construct_ss.str();
}

bool construct_util::is_valid_construction(const ConstructionStatement& s, const struct_info& to_construct_type)
{
    return !(
        !s.get_construction().has_default() &&
        (
            to_construct_type.members_size() != 
            s.get_construction().num_initializations()
        )
    ) && !(
        s.get_construction().has_default() &&
        (
            to_construct_type.members_size() ==
            s.get_construction().num_initializations()
        )
    );
}
