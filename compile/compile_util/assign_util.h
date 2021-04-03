/*

SIN Toolchain (x86 target)
assign_util.h
Copyright 2020 Riley Lannon

Utilities for assignment -- specifically, tools to determine the source and destination operand locations

*/

#pragma once

#include <sstream>
#include <string>
#include <memory>

#include "utilities.h"
#include "expression_util.h"

namespace assign_utilities {
    enum MoveInstruction {
        MOV,
        LEA
    };

    struct destination_information
    {
        std::string dest_location;      // the location
        std::string fetch_instructions; // the instructions to fetch it
        std::string address_for_lea;    // if we need 'lea' (e.g., for strings), we should track the pointer here
        bool in_register;   // whether it's in a register
        bool can_use_lea;   // whether it can use the lea instruction
        MoveInstruction instruction_used;   // the instruction to use (MOV or LEA)

        destination_information(
            const std::string& dest_location,
            const std::string& fetch_instructions,
            const std::string& address_for_lea = "",
            bool in_register=false,
            bool can_use_lea=false,
            MoveInstruction instruction_used = MOV
        );
    };

    destination_information fetch_destination_operand(
        const Expression &exp,
        symbol_table &symbols,
        struct_table &structures,
        const std::string& scope_name,
        unsigned int scope_level,
        unsigned int line,
        reg r = RBX,
        bool is_initialization = false
    );
    destination_information fetch_destination_operand(
        const symbol &sym,
        symbol_table &symbols,
        unsigned int line,
        reg r = RBX,
        bool is_initialization = false
    );
    bool requires_copy(const DataType& t);
    bool is_valid_move_expression(const Expression &exp);

    std::string do_assign(
        const reg src_reg, 
        const DataType& lhs_type, 
        const destination_information& dest, 
        register_usage& context, 
        const unsigned int line,
        bool& do_free
    );
};
