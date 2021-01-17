/*

SIN Toolchain (x86 target)
compile_util/utilities.h

Our various compiler utilities

*/

#pragma once

#include <vector>
#include <string>
#include <sstream>
#include <cinttypes>
#include <unordered_map>

#include "const_symbol.h"
#include "symbol_table.h"
#include "struct_table.h"
#include "../../util/DataType.h"
#include "../../parser/Statement.h" // includes 'Expression.h'
#include "../symbol.h"
#include "../function_symbol.h"
#include "../../util/Exceptions.h"
#include "register_usage.h"
#include "../../util/stack.h"
#include "../../util/data_widths.h"
#include "../struct_info.h"

#include "constant_eval.h"

#include "../../util/general_utilities.h"
#include "magic_numbers.h"

// todo: put these in their own namespace

bool is_valid_cast(DataType &old_type, DataType &new_type);

bool is_subscriptable(Type t);

std::stringstream cast(const DataType &old_type, const DataType &new_type, const unsigned int line, const bool is_strict);

bool can_pass_in_register(DataType to_check);

std::string get_rax_name_variant(DataType t, unsigned int line);

struct_info define_struct(StructDefinition &definition, compile_time_evaluator &cte);

template<typename T>
symbol generate_symbol(T &allocation, size_t data_width, std::string scope_name, unsigned int scope_level, size_t &stack_offset, bool defined=true);

std::stringstream store_symbol(symbol& s);

std::stringstream push_used_registers(register_usage &regs, bool ignore_ab = false);
std::stringstream pop_used_registers(register_usage regs, bool ignore_ab = false);

std::string get_address(symbol &s, reg r);
std::string get_struct_member_address(symbol &struct_symbol, struct_table &structs, std::string member_name, reg r);

std::string decrement_rc(
    register_usage &r,
    symbol_table &symbols,
    struct_table &structs,
    std::string scope,
    unsigned int level,
    bool is_function
);
std::string decrement_rc_util(
    std::vector<symbol> &to_free,
    symbol_table &symbols,
    struct_table &structs,
    std::string scope,
    unsigned int level,
    bool is_function,
    symbol *parent = nullptr
);
