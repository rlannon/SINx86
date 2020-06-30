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

	// todo: use 'extern' quality and pass it to generate_symbol

	std::stringstream allocation_ss;

	DataType &alloc_data = alloc_stmt.get_type_information();
	size_t data_width = alloc_data.get_width();

	// todo: array length needs to be determined for _all_ arrays
	// where it can be determined at compile-time, this space must be reserved on the stack
	// where this is not possible, evaluate the expression and pass it to sinl_array_alloc

	// if the type is 'array', we need to evaluate the array width that was parsed earlier
	if (alloc_data.get_primary() == ARRAY) {
		// if we have an array length expression, and it's a constant, evaluate it
		if (alloc_data.get_array_length_expression() != nullptr && alloc_data.get_array_length_expression()->is_const()) {
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
				alloc_data.set_array_length(stoul(
					this->evaluator.evaluate_expression(
						alloc_data.get_array_length_expression(),
						this->current_scope_name,
						this->current_scope_level,
						alloc_stmt.get_line_number()
					)));
				data_width = alloc_data.get_array_length() * alloc_data.get_full_subtype()->get_width() + sin_widths::INT_WIDTH;
			}
			else {
				throw CompilerException("An array width must be a positive integer", compiler_errors::TYPE_ERROR, alloc_stmt.get_line_number());
			}
		}
		else {
			// if the length is not constant, check to see if we have a dynamic array; if not, then it's not legal
			if (alloc_data.get_qualities().is_dynamic()) {
				alloc_data.set_array_length(0);
				data_width = sin_widths::PTR_WIDTH;
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
	else if (alloc_data.get_primary() == STRUCT) {
		struct_info &info = this->get_struct_info(alloc_data.get_struct_name(), alloc_stmt.get_line_number());
		data_width = info.get_width();
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
			data_width = sin_widths::PTR_WIDTH;
			symbol allocated = generate_symbol(alloc_stmt, data_width, this->current_scope_name, this->current_scope_level, this->max_offset);
			
			// add the symbol and move RSP further into the stack, by the width of a pointer
			this->add_symbol(allocated, alloc_stmt.get_line_number());

			// allocate dynamic memory with a call to sre_request_resource
			allocation_ss << "\t" << "pushfq" << std::endl;
			allocation_ss << "\t" << "push rbp" << std::endl;
			allocation_ss << "\t" << "mov rbp, rsp" << std::endl;
			allocation_ss << "\t" << "mov rdi, " << data_width << std::endl;
			allocation_ss << "\t" << "call sre_request_resource" << std::endl;
			allocation_ss << "\t" << "mov rsp, rbp" << std::endl;
			allocation_ss << "\t" << "pop rbp" << std::endl;
			allocation_ss << "\t" << "popfq" << std::endl;

			// store the returned address in the space allocated for the resource
			allocation_ss << "\t" << "mov [rbp - " << allocated.get_offset() << "], rax" << std::endl;
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
			data_width = 0;	// takes up no space on the stack
			symbol allocated = generate_symbol(alloc_stmt, data_width, "global", 0, this->max_offset);
			this->add_symbol(allocated, alloc_stmt.get_line_number());

			// static data must be marked as 'global'
			allocation_ss << "extern " << allocated.get_name() << std::endl;

			// static const variables can go in the .rodata segment, so check to see if it is also const
			if (alloc_data.get_qualities().is_const()) {
				// todo: static const memory
			}
			else {
				// todo: static, non-const memory
			}
		}
		else {
			// must be automatic memory
			// allocate memory on the stack

			// construct the symbol
			symbol allocated = generate_symbol(alloc_stmt, data_width, this->current_scope_name, this->current_scope_level, this->max_offset);

			/*
			
			move RSP by the width of the type so that we can safely use the stack without overwriting our local variables
			if we have an array or a struct, we need to calculate its width beyond just getting DataType::width
			
			This has to happen here for pointers -- we have to push values *below* the value we just assigned

			*/
			
			size_t to_subtract = 0;

			if (allocated.get_data_type().get_primary() == STRUCT && !allocated.get_data_type().get_qualities().is_dynamic()) {
				struct_info &s = this->get_struct_info(allocated.get_data_type().get_struct_name(), alloc_stmt.get_line_number());
				to_subtract = s.get_width();
			}
			else if (allocated.get_data_type().get_primary() == ARRAY && !allocated.get_data_type().get_qualities().is_dynamic()) {
				to_subtract = allocated.get_data_type().get_array_length() * allocated.get_data_type().get_full_subtype()->get_width() + sin_widths::INT_WIDTH;
				
				// write the array length onto the stack
				allocation_ss << "\t" << "mov eax, " << allocated.get_data_type().get_array_length() << std::endl;
				allocation_ss << "\t" << "mov [rbp - " << allocated.get_offset() << "], eax" << std::endl;
			}
			else {
				to_subtract = allocated.get_data_type().get_width();
			}

			allocation_ss << "\t" << "sub rsp, " << to_subtract << std::endl;

			// if the type is string, we need to call sinl_string_alloc
			if (alloc_data.get_primary() == STRING) {
				// todo: get string length instead of passing 0 in
				allocation_ss << "\t" << "mov esi, 0" << std::endl;
				allocation_ss << "\t" << "push rbp" << std::endl;
				allocation_ss << "\t" << "mov rbp, rsp" << std::endl;
				allocation_ss << "\t" << "call sinl_string_alloc" << std::endl;
				allocation_ss << "\t" << "mov rsp, rbp" << std::endl;
				allocation_ss << "\t" << "pop rbp" << std::endl;
				
				// save the location of the string
				allocation_ss << "\t" << "mov [rbp - " << allocated.get_offset() << "], rax" << std::endl;
			}

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
		}
	}
	else {
		throw TypeValidityViolation(alloc_stmt.get_line_number());	// todo: generate a more specific error saying what the policy violation was
	}

    // return our allocation code
    return allocation_ss;
}
