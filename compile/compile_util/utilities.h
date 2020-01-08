/*

SIN Toolchain (x86 target)
compile_util/utilities.h

Our various compiler utilities

*/

#pragma once

#include <unordered_map>
#include <sstream>

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

// todo: make these into friend classes so they can access the symbol table? pass symbol table as parameter?
// todo: pass in arguments list
std::stringstream generate_call_header(function_symbol s, unsigned int line);
std::stringstream generate_sincall(function_symbol s, unsigned int line);
