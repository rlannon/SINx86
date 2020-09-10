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
        std::string dest_location;
        std::string fetch_instructions;
        std::string address_for_lea;    // if we need 'lea' (e.g., for strings), we should track the pointer here
        bool in_register;
        bool can_use_lea;
        MoveInstruction instruction_used;

        destination_information(
            std::string dest_location,
            std::string fetch_instructions,
            std::string address_for_lea = "",
            bool in_register=false,
            bool can_use_lea=false,
            MoveInstruction instruction_used = MOV
        );
    };

    destination_information fetch_destination_operand(
        std::shared_ptr<Expression> exp,
        symbol_table &symbols,
        struct_table &structures,
        std::string scope_name,
        unsigned int scope_level,
        unsigned int line,
        reg r = RBX,
        bool is_initialization = false
    );
    destination_information fetch_destination_operand(
        symbol &sym,
        symbol_table &symbols,
        unsigned int line,
        reg r = RBX,
        bool is_initialization = false
    );
    bool requires_copy(DataType t);
    bool is_valid_move_expression(std::shared_ptr<Expression> exp);
};
