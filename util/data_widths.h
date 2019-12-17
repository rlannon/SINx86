/*

SIN Toolchain (x86 target)
data_widths.h

Contains the widts for our various data types.

*/

#include <cinttypes>
#include <cstdlib>

namespace sin_widths {
    const size_t INT_WIDTH = 4;
    const size_t LONG_WIDTH = 8;
    const size_t SHORT_WIDTH = 2;

    const size_t BOOL_WIDTH = 1;

    const size_t PTR_WIDTH = 8;

    const size_t FLOAT_WIDTH = 4;
    const size_t DOUBLE_WIDTH = 8;
    const size_t HALF_WIDTH = 2;
};
