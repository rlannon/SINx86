/*

SIN Toolchain (x86 target)
data_widths.h

Contains the width constants for our various data types.

*/

#pragma once

#include <cinttypes>
#include <cstdlib>
#include <unordered_map>

#include "EnumeratedTypes.h"

namespace sin_widths {
	// Note that all widths here are given in _bytes_, not in _bits_

    const size_t INT_WIDTH = 4;
    const size_t LONG_WIDTH = 8;
    const size_t SHORT_WIDTH = 2;

    const size_t BOOL_WIDTH = 1;
	const size_t CHAR_WIDTH = 1;

    const size_t PTR_WIDTH = 8;

    const size_t FLOAT_WIDTH = 4;
    const size_t DOUBLE_WIDTH = 8;
    const size_t HALF_WIDTH = 2;
};
