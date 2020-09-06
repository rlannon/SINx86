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
	
	// todo: generate a warning about divergent references if we try to create a pointer or reference to a dynamic type

	std::stringstream allocation_ss;

	DataType &alloc_data = alloc_stmt.get_type_information();
	size_t data_width = expression_util::get_width(
		alloc_data,
		this->evaluator,
		this->structs,
		this->symbols,
		this->current_scope_name,
		this->current_scope_level,
		alloc_stmt.get_line_number()
	);

	// the first thing we need to do it check to make sure the type was initialized, if required
	if (alloc_data.get_primary() == STRUCT) {
		// structs just defer checking members until later -- we have to do it after allocation
	}
	else if (alloc_data.get_primary() == TUPLE) {
		// todo: how to handle tuples?
	}
	else if (alloc_data.must_initialize() && !alloc_stmt.was_initialized()) {
		// throw an exception based on the type/qualities
		if (alloc_data.get_qualities().is_const()) {
			throw ConstAllocationException(alloc_stmt.get_line_number());
		}
		else if (alloc_data.get_primary() == REFERENCE) {
			throw CompilerException(
				"Reference not initialized",
				compiler_errors::REFERENCE_ALLOCATION_ERROR,
				alloc_stmt.get_line_number()
			);
		}
		else {
			throw CompilerException(
				"Data must be initialized in allocation",
				compiler_errors::ALLOC_INIT_REQUIRED,
				alloc_stmt.get_line_number()
			);
		}
	}

	// todo: struct allocation -- allocate each member accordingly

	// todo: array length needs to be determined for _all_ arrays
	// where it can be determined at compile-time, this space must be reserved on the stack
	// where this is not possible, evaluate the expression and pass it to sinl_array_alloc

	// allocate the variable if our type was valid
	if (DataType::is_valid_type(alloc_data)) {
		symbol allocated;

		// variables in the global scope do not need to be marked as 'static' by the programmer, though they are located in static memory so we must set the static quality if we are in the global scope
		if (this->current_scope_name == "global") {
			alloc_data.get_qualities().add_quality(SymbolQuality::STATIC);
		}

		// we have some special things we need to do when we allocate constants
		if (alloc_data.get_qualities().is_const()) {
			// todo: evaluate the constant and add it to the constant table
		}

		// perform the allocation
		if (alloc_data.get_qualities().is_dynamic()) {
			// dynamic allocation -- ensure we pass PTR_WIDTH in for the width (as it will affect the stack offset)
			allocated = generate_symbol(alloc_stmt, sin_widths::PTR_WIDTH, this->current_scope_name, this->current_scope_level, this->max_offset);

			// push registers currently in use
			allocation_ss << push_used_registers(this->reg_stack.peek(), true).str();

			// allocate dynamic memory with a call to _sre_request_resource
			allocation_ss << "\t" << "mov rdi, " << data_width << std::endl;
			allocation_ss << call_sre_function("_sre_request_resource");

			// restore used registers
			allocation_ss << pop_used_registers(this->reg_stack.peek(), true).str();

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

			// ensure we handle alloc-init for dynamic objects
			if (alloc_stmt.was_initialized()) {
				auto initial_value = alloc_stmt.get_initial_value();
				allocation_ss << this->handle_alloc_init(allocated, initial_value, alloc_stmt.get_line_number()).str();

				allocated.set_initialized();
			}

			// add the symbol
			this->add_symbol(allocated, alloc_stmt.get_line_number());
		}
		else if (alloc_data.get_qualities().is_static()) {
			data_width = 0;	// takes up no space on the stack
			allocated = generate_symbol(alloc_stmt, data_width, "global", 0, this->max_offset);

			// we need to determine the width suffix (db, dw, resb, resw, etc)
			size_t w = allocated.get_data_type().get_width();

			// since arrays must contain a known-width type, we can just get the width of the subtype
			if (allocated.get_data_type().get_primary() == ARRAY)
				w = allocated.get_data_type().get_subtype().get_width();
			
			char width_suffix;
			if (w == sin_widths::BOOL_WIDTH) {
				width_suffix = 'b';
			}
			else if (w == sin_widths::SHORT_WIDTH) {
				width_suffix = 'w';
			}
			else if (w == sin_widths::INT_WIDTH) {
				width_suffix = 'd';
			}
			else {
				width_suffix = 'q';
			}

			// get the initial value of the static memory, if we have one we can use
			std::string initial_value = "";
			if (alloc_stmt.was_initialized() && alloc_stmt.get_initial_value()->is_const()) {
				initial_value = this->evaluator.evaluate_expression(
					alloc_stmt.get_initial_value(), 
					"global", 
					0, 
					alloc_stmt.get_line_number()
				);
			}
			// if the data is const and we don't have a const initial value, it's an error
			else if (alloc_data.get_qualities().is_const()) {
				throw ConstInitializationException(alloc_stmt.get_line_number());
			}
			std::stringstream alloc_instruction;

			// form the instruction
			if (alloc_data.get_primary() == ARRAY) {
				alloc_instruction << allocated.get_name() << " dd " <<
					std::to_string(allocated.get_data_type().get_array_length()) << std::endl;
				
				// if the array was initialized, supply our array; else, initialize to a zeroed array
				if (alloc_stmt.was_initialized()) {
					alloc_instruction << "d" << width_suffix << " " << initial_value << std::endl;
				}
				else {
					alloc_instruction << "times " << allocated.get_data_type().get_array_length() <<
						" d" << width_suffix << " 0" << std::endl;
				}
			}
			else if (alloc_data.get_primary() == STRUCT) {
				alloc_instruction << allocated.get_name() << " times " << data_width << " db 0" << std::endl;
			}
			else {
				if (alloc_stmt.was_initialized()) {
					alloc_instruction << allocated.get_name() << " d" << width_suffix << " " << initial_value << std::endl;
				}
				else {
					alloc_instruction << allocated.get_name() << " res" << width_suffix << " 1" << std::endl;
				}
			}

			// static const variables can go in the .rodata segment, so check to see if it is also const
			if (alloc_data.get_qualities().is_const()) {
				// static const memory
				this->rodata_segment << alloc_instruction.str() << std::endl;
			}
			else if (
				(allocated.was_initialized() && alloc_stmt.get_initial_value()->is_const()) ||
				alloc_data.get_primary() == ARRAY
			) {
				// static, non-const, initialized data
				this->data_segment << alloc_instruction.str() << std::endl;
			}
			else if (allocated.was_initialized()) {
				// if we have an initial value that is not constant, throw an exception; that isn't allowed
				throw CompilerException(
					"Static data must be initialized to a compile-time constant or not at all (default initialized to 0)",
					compiler_errors::STATIC_MEMORY_INITIALIZATION_ERROR,
					alloc_stmt.get_line_number()
				);
			}
			else {
				// static, non-const, uninitialized data
				this->bss_segment << alloc_instruction.str() << std::endl;
			}

			// add the symbol to the table
			if (alloc_stmt.was_initialized()) allocated.set_initialized();
			this->add_symbol(allocated, alloc_stmt.get_line_number());
		}
		else {
			// must be automatic memory
			// allocate memory on the stack

			// construct the symbol
			allocated = generate_symbol(alloc_stmt, data_width, this->current_scope_name, this->current_scope_level, this->max_offset);

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
				to_subtract = allocated.get_data_type().get_array_length() * allocated.get_data_type().get_subtype().get_width() + sin_widths::INT_WIDTH;
				
				// write the array length onto the stack
				allocation_ss << "\t" << "mov eax, " << allocated.get_data_type().get_array_length() << std::endl;
				allocation_ss << "\t" << "mov [rbp - " << allocated.get_offset() << "], eax" << std::endl;
			}
			else {
				to_subtract = allocated.get_data_type().get_width();
			}

			// if the type is string, we need to call sinl_string_alloc
			if (alloc_data.get_primary() == STRING) {
				// preserve registers
				allocation_ss << push_used_registers(this->reg_stack.peek(), true).str();

				allocation_ss << "\t" << "sub rsp, " << to_subtract << std::endl;

				// todo: get string length instead of passing 0 in
				allocation_ss << "\t" << "mov esi, 0" << std::endl;
				allocation_ss << call_sincall_subroutine("sinl_string_alloc");

				allocation_ss << "\t" << "add rsp, " << to_subtract << std::endl;
				
				// restore used registers
				allocation_ss << pop_used_registers(this->reg_stack.peek(), true).str();

				// save the location of the string
				allocation_ss << "\t" << "mov [rbp - " << allocated.get_offset() << "], rax" << std::endl;
			}
			// todo: utilize sinl_string_copy_construct for alloc-init with strings

			// subtract the width of the type from RSP
			allocation_ss << "\t" << "sub rsp, " << data_width << std::endl;

			// initialize it, if necessary
			if (alloc_stmt.was_initialized()) {
				// get the initial value
				std::shared_ptr<Expression> initial_value = alloc_stmt.get_initial_value();

				// make an assignment of 'initial_value' to 'allocated'
				allocation_ss << this->handle_alloc_init(allocated, initial_value, alloc_stmt.get_line_number()).str();

				// mark the symbol as initialized
				allocated.set_initialized();
			}

			// add it to the table
			this->add_symbol(allocated, alloc_stmt.get_line_number());
		}

		// if the type is STRUCT, we need to initialize non-dynamic array members
		if (allocated.get_data_type().get_primary() == STRUCT) {
			// todo: eliminate this call and initialize a function-level object earlier?
			struct_info &info = this->get_struct_info(allocated.get_data_type().get_struct_name(), alloc_stmt.get_line_number());
			
			// get a register for the address
			reg r = this->reg_stack.peek().get_available_register(PTR);
			bool push_r15 = false;
			if (r == NO_REGISTER) {
				symbol *contained = this->reg_stack.peek().get_contained_symbol(R15);
				if (contained) {
					allocation_ss << store_symbol(*contained).str();
					contained->set_register(NO_REGISTER);
					this->reg_stack.peek().clear_contained_symbol(R15);
				}
				else {
					push_r15 = true;
					allocation_ss << "\t" << "push r15" << std::endl;
				}

				r = R15;
			}

			// if we have a struct, iterate through its members and initialize the array lengths
			// we also need to make sure that we are following construction rules
			bool init_required = false;	// if the struct contains references, it must be initialized in the allocation
			auto members = info.get_all_members();
			std::string struct_addr = get_address(allocated, r);
			allocation_ss << struct_addr;
			for (auto m: members) {
				// we only need to do this for non-dynamic arrays
				if (m->get_data_type().get_primary() == ARRAY && !m->get_data_type().get_qualities().is_dynamic()) {
					// evaluate the array length expression and move it (an integer) into [R15 + offset]
					auto alloc_p = this->evaluate_expression(m->get_data_type().get_array_length_expression(), alloc_stmt.get_line_number());
					allocation_ss << alloc_p.first;
					allocation_ss << "\t" << "mov [" << register_usage::get_register_name(r) << " + " << m->get_offset() << "], eax" << std::endl;

					// if we had a reference to an integer, we need to free it
					if (alloc_p.second) {
						allocation_ss << "\t" << "pop rdi" << std::endl;
						allocation_ss << call_sre_function("_sre_free");
					}
				}
				else if (m->get_data_type().must_initialize()) {
					init_required = true;
				}
			}

			// if we needed to initialize but didn't, throw an exception
			if (init_required && !alloc_stmt.was_initialized()) {
				throw CompilerException(
					"Struct '" + info.get_struct_name() + "' must be initialized when allocated because it contains one or more members that require it (hint: use 'construct')",
					compiler_errors::ALLOC_INIT_REQUIRED,
					alloc_stmt.get_line_number()
				);
			}

			// if we had to push r15, restore it
			if (push_r15) {
				allocation_ss << "\t" << "pop r15" << std::endl;
			}
		}
	}
	else {
		throw TypeValidityViolation(alloc_stmt.get_line_number());	// todo: generate a more specific error saying what the policy violation was
	}

    // return our allocation code
    return allocation_ss;
}
