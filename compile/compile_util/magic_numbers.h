#pragma once

/*

magic_numbers.h
Some magic numbers for our compiler

*/

#include <string>

namespace magic_numbers {
    // Some magic numbers

    // Macros have already been defined for our SRE names in assembly
    const std::string SRE_INIT = "%[SRE_INIT]";
    const std::string SRE_CLEAN = "%[SRE_CLEAN]";
    const std::string SRE_REQUEST_RESOURCE = "%[SRE_REQUEST_RESOURCE]";
    const std::string SRE_REALLOCATE = "%[SRE_REALLOCATE]";
    const std::string SRE_ADD_REF = "%[SRE_ADD_REF]";
    const std::string SRE_FREE = "%[SRE_FREE]";
    const std::string SINL_RTE_OUT_OF_BOUNDS = "%[SINL_RTE_OUT_OF_BOUNDS]";

    // Magic numbers for other things
    const std::string CONST_STRING_LABEL = "sinl_strc_";
    const std::string LIST_LITERAL_LABEL = "sinl_list_";
    const std::string FLOAT_LITERAL_LABEL = "sinl_fltc_";
    const std::string ITE_LABEL = ".sinl_ite_";
    const std::string ITE_ELSE_LABEL = ".sinl_ite_else_";
    const std::string ITE_DONE_LABEL = ".sinl_ite_done_";
    const std::string WHILE_LABEL = ".sinl_while_";
    const std::string WHILE_DONE_LABEL = ".sinl_while_done_";
    const std::string SINGLE_PRECISION_MASK_LABEL = "sinl_sp_mask";
    const std::string DOUBLE_PRECISION_MASK_LABEL = "sinl_dp_mask";

    const std::string MAIN_LABEL = "%[SIN_MAIN]";
}
