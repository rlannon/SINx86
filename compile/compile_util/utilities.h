/*

SIN Toolchain (x86 target)
compile_util/utilities.h

Our various compiler utilities

*/

#pragma once

#include <unordered_map>

#include "../../util/DataType.h"
#include "../../parser/Expression.h"
#include "data_widths.h"
#include "../symbol.h"
#include "../../util/Exceptions.h"

DataType get_expression_data_type(std::shared_ptr<Expression> to_eval, std::unordered_map<std::string, std::shared_ptr<symbol>> &symbol_table, unsigned int line);
