/*

SIN Toolchain (x86 target)
allocation.cpp

Handle allocations for the compiler class.

*/

#include <sstream>
#include "compiler.h"

// todo: struct allocations -- when a struct is allocated, it should allocate all of its data members -- like a primitive form of a constructor; when free is called on a struct, it will free _all_ data, but dynamic data will not be freed when the struct goes out of scope

std::stringstream compiler::allocate(Allocation alloc_stmt) {
	/*
    
	allocate
	Dispatches the allocation to the appropriate function
    
	@param	alloc_stmt	The statement containing the allocation
	@returns	A stringstream containing the generated code

	*/

	// todo: advance rsp so that we may safely push data to the stack even when we are allocating local data
    
	DataType alloc_data = alloc_stmt.get_type_information();
    std::stringstream allocation_ss;

	// if the type is 'array', we need to evaluate the array width that was parsed earlier
	if (alloc_data.get_primary() == ARRAY) {
		// if it's a constant, evaluate it
		if (alloc_data.get_array_length_expression()->is_const()) {
			if (
				get_expression_data_type(
					alloc_data.get_array_length_expression(),
					this->symbols,
					this->structs,
					alloc_stmt.get_line_number()
				).get_primary() == INT)
			{
				// pass the expression to our expression evaluator to get the array width
				// todo: compile-time evaluation
				// todo: set alloc_data::array_length
			}
			else {
				throw CompilerException("An array width must be a positive integer", compiler_errors::TYPE_ERROR, alloc_stmt.get_line_number());
			}
		}
		else {
			// if the length is not constant, check to see if we have a dynamic array; if not, then it's not legal
			if (alloc_data.get_qualities().is_dynamic()) {
				alloc_data.set_array_length(0);
			}
			else {
				throw CompilerException(
					"The length of a non-dynamic array must be known at compile time (use a literal or a valid constexpr)",
					compiler_errors::TYPE_VALIDITY_RULE_VIOLATION_ERROR,
					alloc_stmt.get_line_number()
				);
			}
		}
	}

	// allocate the variable if our type was valid
	if (DataType::is_valid_type(alloc_data)) {
		// variables in the global scope do not need to be marked as 'static' by the programmer, though they are located in static memory so we must set the static quality if we are in the global scope
		if (this->current_scope_name == "global") {
			alloc_data.get_qualities().add_quality(SymbolQuality::STATIC);
		}

		// we have some special things we need to do when we allocate constants
		if (alloc_data.get_qualities().is_const()) {
			// check to ensure it was initialized
			if (!alloc_stmt.was_initialized())
				throw ConstAllocationException(alloc_stmt.get_line_number());

			// todo: evaluate the constant and add it to the constant table
		}

		// perform the allocation
		if (alloc_data.get_qualities().is_dynamic()) {
			// dynamic allocation
			symbol allocated = generate_symbol(alloc_stmt, this->current_scope_name, this->current_scope_level, this->max_offset);
			
			// add the symbol and move RSP further into the stack, by the width of a pointer
			this->add_symbol(allocated, alloc_stmt.get_line_number());
			allocation_ss << "\t" << "sub rsp, " << sin_widths::PTR_WIDTH << std::endl;

			// if we have a const here, throw an exception -- constants may not be dynamic
			if (alloc_data.get_qualities().is_const()) {
				throw CompilerException("Use of 'const' and 'dynamic' together is illegal", compiler_errors::ILLEGAL_QUALITY_ERROR, alloc_stmt.get_line_number());
			}

			// if we have static, that should also generate an error
			if (alloc_data.get_qualities().is_static()) {
				throw CompilerException("Use of 'static' and 'dynamic' together is illegal", compiler_errors::ILLEGAL_QUALITY_ERROR, alloc_stmt.get_line_number());
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
			else {

			}
		}
		else {
			// must be automatic memory
			// allocate memory on the stack

			// construct the symbol
			symbol allocated = generate_symbol(alloc_stmt, this->current_scope_name, this->current_scope_level, this->max_offset);

			// initialize it, if necessary
			if (alloc_stmt.was_initialized()) {
				// get the initial value
				std::shared_ptr<Expression> initial_value = alloc_stmt.get_initial_value();

				// make an assignment of 'initial_value' to 'allocated'
				allocation_ss << this->handle_symbol_assignment(allocated, initial_value, alloc_stmt.get_line_number()).str();

				// mark the symbol as initialized
				allocated.set_initialized();
			}

			// add it to the table
			this->add_symbol(allocated, alloc_stmt.get_line_number());

			/*
			
			now, move RSP by the width of the type so that we can safely use the stack without overwriting our local variables
			if we have an array or a struct, we need to calculate its width beyond just getting DataType::width
			
			*/
			
			size_t to_subtract = 0;

			if (allocated.get_data_type().get_primary() == STRUCT && !allocated.get_data_type().get_qualities().is_dynamic()) {
				struct_info &s = this->get_struct_info(allocated.get_data_type().get_struct_name(), alloc_stmt.get_line_number());
				to_subtract = s.get_width();
			}
			else if (allocated.get_data_type().get_primary() == ARRAY && !allocated.get_data_type().get_qualities().is_dynamic()) {
				to_subtract = allocated.get_data_type().get_array_length() * allocated.get_data_type().get_width() + sin_widths::INT_WIDTH;
			}
			else {
				to_subtract = allocated.get_data_type().get_width();
			}

			allocation_ss << "\t" << "sub rsp, " << to_subtract << std::endl;
		}
	}
	else {
		throw TypeValidityViolation(alloc_stmt.get_line_number());	// todo: generate a more specific error saying what the policy violation was
	}

    // return our allocation code
    return allocation_ss;
}
