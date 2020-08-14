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
#include "member_selection.h"

#include "constant_eval.h"

#include "../../util/general_utilities.h"

// todo: put these in their own namespace

std::string call_sincall_subroutine(std::string name);

bool returns(StatementBlock to_check);

bool is_valid_cast(DataType &old_type, DataType &new_type);

bool is_subscriptable(Type t);

std::stringstream cast(DataType &old_type, DataType &new_type, unsigned int line);

bool can_pass_in_register(DataType to_check);

std::string get_rax_name_variant(DataType t, unsigned int line);

struct_info define_struct(StructDefinition definition, compile_time_evaluator &cte);

template<typename T>
function_symbol create_function_symbol(T def, bool mangle=true, bool defined=true);

template<typename T>
symbol generate_symbol(T &allocation, size_t data_width, std::string scope_name, unsigned int scope_level, size_t &stack_offset, bool defined=true);

std::stringstream store_symbol(symbol& s);

std::stringstream push_used_registers(register_usage &regs, bool ignore_ab = false);
std::stringstream pop_used_registers(register_usage regs, bool ignore_ab = false);

std::string get_address(symbol &s, reg r);

std::stringstream copy_array(symbol &src, symbol &dest, register_usage &regs);
std::stringstream copy_string(symbol &src, symbol &dest, register_usage &regs);

std::stringstream decrement_rc(register_usage &r, symbol_table& t, std::string scope, unsigned int level, bool is_function);

std::stringstream call_sre_free(symbol& s);
std::stringstream call_sre_add_ref(symbol& s);
std::stringstream call_sre_mam_util(symbol& s, std::string func_name);
std::string call_sre_function(std::string func_name);
