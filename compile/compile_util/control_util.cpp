/*

SIN toolchain (x86 target)
control_util.cpp
Copyright 2021 Riley Lannon

*/

#include <sstream>

#include "control_util.h"
#include "expression_util.h"

std::string control_util::restore_register_variables(
    register_usage& leaving,
    register_usage& entering,
    const std::string& entering_scope_name,
    const unsigned int entering_scope_level
) {
    /*

    restore_register_variables
    Saves and restore variables when switching between register contexts

    The general algorithm goes as follows, for each register in the context we are leaving:
        * If the register is not in use, skip it
        * If it is in use, check to see if it is in use in the context we are entering
            * If not:
                * If the variable is still accessible, store it
                * Clear the register
            * If so:
                * If the contained symbol is the same, continue
                * If the symbol is different, store the current symbol (if accessible)
                  and restore the old one        

    */

    std::stringstream gen_code;

    for (auto reg_it = entering.all_regs.begin(); reg_it != entering.all_regs.end(); reg_it++)
    {
        // we only care about this register if it's being used in the context we are leaving
        if (leaving.is_in_use(*reg_it))
        {
            if (entering.is_in_use(*reg_it))
            {
                auto leaving_sym = leaving.get_contained_symbol(*reg_it);
                auto entering_sym = leaving.get_contained_symbol(*reg_it);

                if ((leaving_sym && entering_sym) && (*leaving_sym != *entering_sym))
                {
                    // we only need to store this symbol if it's still accessible
                    if (leaving_sym->is_accessible_from(entering_scope_name, entering_scope_level))
                    {
                        gen_code << store_symbol(*leaving_sym).str();
                        leaving_sym->set_register(reg::NO_REGISTER);
                    }

                    // now, reload the register
                    // note that because this function will transfer the symbol
                    gen_code << expression_util::load_into_register(*entering_sym, *reg_it, entering);
                }
            }
            else
            {
                auto contained = leaving.get_contained_symbol(*reg_it);
                if (contained && contained->is_accessible_from(entering_scope_name, entering_scope_level))
                {
                    gen_code << store_symbol(*contained).str();
                }

                leaving.clear(*reg_it);
            }
        }
    }

    return gen_code.str();
}
