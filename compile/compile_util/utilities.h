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

#include "../../util/DataType.h"
#include "../../parser/Statement.h" // includes 'Expression.h'
#include "../symbol.h"
#include "../../util/Exceptions.h"
#include "register_usage.h"
#include "../../util/stack.h"
#include "../../util/data_widths.h"
#include "../struct_info.h"

DataType get_expression_data_type(std::shared_ptr<Expression> to_eval, std::unordered_map<std::string, std::shared_ptr<symbol>> &symbol_table, unsigned int line);

bool can_pass_in_register(DataType to_check);

struct_info define_struct(StructDefinition definition);

template<class T>
function_symbol create_function_symbol(T def);

template<class T>
symbol generate_symbol(T &allocation, std::string scope_name, unsigned int scope_level, size_t &stack_offset);
