/*

SIN Toolchain (x86 target)
allocation.cpp

Handle allocations for the compiler class.

*/

#include <sstream>
#include "compile.h"

std::stringstream compiler::allocate(Allocation alloc_stmt) {
    // Dispatches the allocation to the appropriate function
    
    DataType alloc_data = alloc_stmt.get_type_information();
    std::stringstream allocation_ss;

    if (alloc_data.get_qualities().is_dynamic()) {
        // todo: allocate dynamically
    } else if (alloc_data.get_qualities().is_const()) {
        // todo: allocate const memory
    } else if (alloc_data.get_qualities().is_static()) {
        // todo: allocate static memory
    } else {
        // must be automatic memory
        // allocate memory on the stack
        allocation_ss << allocate_automatic(alloc_stmt).str();
        if (alloc_stmt.was_initialized()) {
            // todo: make initial assignment
        }
    }
}

std::stringstream compiler::allocate_automatic(Allocation alloc_stmt) {
    // Allocates automatic memory according to the allocation statement provided

    // We need to create a symbol appropriately
    symbol to_allocate(alloc_stmt.get_var_name(), this->current_scope_name, this->current_scope_level, alloc_stmt.get_type_information(), this->max_offset);
    std::pair<std::string, symbol> to_insert(to_allocate.get_name(), to_allocate);
    this->symbol_table.insert(to_insert);

    // update the max offset
    this->max_offset += alloc_stmt.get_type_information().get_width();

    // Generates no code, so it always returns an empty stringstream
    return std::stringstream("");
}
