/*

SIN Toolchain (x86 target)
allocation.cpp

Handle allocations for the compiler class.

*/

#include <sstream>
#include "compiler.h"

// todo: struct allocations -- when a struct is allocated, it should allocate all of its data members -- like a primitive form of a constructor; when free is called on a struct, it will free _all_ data, but dynamic data will not be freed when the struct goes out of scope

std::stringstream compiler::allocate(Allocation alloc_stmt) {
    // Dispatches the allocation to the appropriate function
    
    DataType alloc_data = alloc_stmt.get_type_information();
    std::stringstream allocation_ss;

    // variables in the global scope do not need to be marked as 'static' by the programmer, though they are located in static memory so we must set the static quality if we are in the global scope
    if (this->current_scope_name == "global") {
        alloc_data.get_qualities().add_quality(SymbolQuality::STATIC);
    }

    // now we may make the assignment
    if (alloc_data.get_qualities().is_dynamic()) {
        // todo: allocate dynamically
    } else if (alloc_data.get_qualities().is_const()) {
        // todo: allocate const memory
    } else if (alloc_data.get_qualities().is_static()) {
        // todo: allocate static memory
    } else {
        // must be automatic memory
        // allocate memory on the stack

        // construct the symbol with our utility function and add it to the symbol table
        symbol allocated = generate_symbol(alloc_stmt, this->current_scope_name, this->current_scope_level, this->max_offset);
        this->add_symbol(allocated, alloc_stmt.get_line_number());

        // initialize it, if necessary
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
