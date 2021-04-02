#pragma once

/*

SIN toolchain (x86 target)
control_util.h
Copyright 2021 Riley Lannon

Utilities for control flow functions

*/

#include <string>

#include "register_usage.h"

namespace control_util
{
    std::string restore_register_variables(
        register_usage& leaving,
        register_usage& entering,
        const std::string& entering_scope_name,
        const unsigned int entering_scope_level
    );
}
