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
        symbol allocated = allocate_automatic(alloc_stmt);
        if (alloc_stmt.was_initialized()) {
            // get the initial value
            std::shared_ptr<Expression> initial_value = alloc_stmt.get_initial_value();

            // make an assignment of 'initial_value' to 'allocated'
            allocation_ss << this->handle_assignment(allocated, initial_value, alloc_stmt.get_line_number()).str();
        }

        // do not return yet in case we have any other code we wish to add later
    }

    // return our allocation code
    return allocation_ss;
}

symbol compiler::allocate_automatic(Allocation alloc_stmt) {
    // Allocates automatic memory according to the allocation statement 
    // Returns a copy of the symbol that was allocated

    // We need to create a symbol appropriately
    symbol to_allocate(alloc_stmt.get_var_name(), this->current_scope_name, this->current_scope_level, alloc_stmt.get_type_information(), this->max_offset);
    std::pair<std::string, std::shared_ptr<symbol>> to_insert(to_allocate.get_name(), std::make_shared<symbol>(to_allocate));
    this->symbol_table.insert(to_insert);

    // update the max offset
    this->max_offset += alloc_stmt.get_type_information().get_width();

    // an automatic allocation generates no code, so return the allocated symbol
    return to_allocate;
}
