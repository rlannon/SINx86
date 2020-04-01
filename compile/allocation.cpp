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

	// allocate the variable if our type was valid
	if (DataType::is_valid_type(alloc_data)) {
		// variables in the global scope do not need to be marked as 'static' by the programmer, though they are located in static memory so we must set the static quality if we are in the global scope
		if (this->current_scope_name == "global") {
			alloc_data.get_qualities().add_quality(SymbolQuality::STATIC);
		}

		// if a constant is to be allocated but no initial value was given, generate an error
		if (alloc_data.get_qualities().is_const() && !alloc_stmt.was_initialized()) {
			throw ConstAllocationException(alloc_stmt.get_line_number());
		}

		// perform the allocation
		if (alloc_data.get_qualities().is_dynamic()) {
			// todo: allocate dynamically

			symbol allocated = generate_symbol(alloc_stmt, this->current_scope_name, this->current_scope_level, this->max_offset);
			this->add_symbol(allocated, alloc_stmt.get_line_number());

			// if we have a const here, throw an exception -- constants may not be dynamic
			if (alloc_data.get_qualities().is_const()) {
				throw CompilerException("Use of 'const' and 'dynamic' together is illegal", compiler_errors::ILLEGAL_QUALITY_ERROR, alloc_stmt.get_line_number());
			}
		}
		else if (alloc_data.get_qualities().is_static()) {
			// todo: allocate static memory
			
			symbol allocated = generate_symbol(alloc_stmt, "global", 0, this->max_offset);
			this->add_symbol(allocated, alloc_stmt.get_line_number());

			// static const variables can go in the .rodata segment, so check to see if it is also const
			if (alloc_data.get_qualities().is_const()) {
				// todo: static const memory
			}
		}
		else {
			// must be automatic memory
			// allocate memory on the stack

			// construct the symbol and add it to the symbol table
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
	}
	else {
		throw TypeValidityViolation(alloc_stmt.get_line_number());	// todo: generate a more specific error saying what the policy violation was
	}

    // return our allocation code
    return allocation_ss;
}
