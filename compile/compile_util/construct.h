#pragma once

/*

SIN toolchain (x86 target)
construct.h
Copyright 2021 Riley Lannon

Construction-related funtionality (the 'construct' keyword)

*/

#include <string>

#include "../symbol.h"
#include "symbol_table.h"
#include "../struct_info.h"

namespace construct_util
{
    std::string default_construct(const symbol& sym, symbol_table& symbols, register_usage& context, const unsigned int line);
    bool is_valid_construction(const ConstructionStatement& s, const struct_info& to_construct_type);
}
