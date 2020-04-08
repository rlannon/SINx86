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
#include "../../util/DataType.h"
#include "../../parser/Statement.h" // includes 'Expression.h'
#include "../symbol.h"
#include "../function_symbol.h"
#include "../../util/Exceptions.h"
#include "register_usage.h"
#include "../../util/stack.h"
#include "../../util/data_widths.h"
#include "../struct_info.h"

// todo: put these in their own namespace

DataType get_expression_data_type(std::shared_ptr<Expression> to_eval, symbol_table& symbols, unsigned int line);

bool returns(StatementBlock &to_check);

bool is_valid_type_promotion(symbol_qualities left, symbol_qualities right);

bool can_pass_in_register(DataType to_check);

std::string get_rax_name_variant(DataType t, unsigned int line);

struct_info define_struct(StructDefinition definition);

template<typename T>
function_symbol create_function_symbol(T def);

template<typename T>
symbol generate_symbol(T &allocation, std::string scope_name, unsigned int scope_level, size_t &stack_offset);

std::stringstream push_used_registers(register_usage regs, bool ignore_ab = false);
std::stringstream pop_used_registers(register_usage regs, bool ignore_ab = false);

std::stringstream copy_array(DataType array_type);
