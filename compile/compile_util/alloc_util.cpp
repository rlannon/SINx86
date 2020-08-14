/*

SIN Toolchain (x86 target)
alloc_util.cpp
Copyright 2020 Riley Lannon

Implementation of the allocation utilities

*/

#include "alloc_util.h"

size_t alloc_util::get_width(
    DataType &alloc_data,
    compile_time_evaluator evaluator,
    struct_table &structs,
    symbol_table &symbols,
    std::string scope_name,
    unsigned int scope_level,
    unsigned int line
) {
    /*

    get_width
    Gets the width of the given data type

    */

    size_t width = 0;

    if (alloc_data.get_width() != 0) {
        width = alloc_data.get_width();
    }
    else if (alloc_data.get_primary() == STRUCT) {
        struct_info &s = structs.find(alloc_data.get_struct_name(), line);
        width = s.get_width();
    }
    else if (alloc_data.get_primary() == ARRAY) {
        // if we have an array length expression, and it's a constant, evaluate it
		if (alloc_data.get_array_length_expression() != nullptr && alloc_data.get_array_length_expression()->is_const()) {
			if (
				expression_util::get_expression_data_type(
					alloc_data.get_array_length_expression(),
					symbols,
					structs,
					line
				).get_primary() == INT)
			{
				// pass the expression to our expression evaluator to get the array width
				// todo: compile-time evaluation
				// todo: set alloc_data::array_length
				alloc_data.set_array_length(stoul(
					evaluator.evaluate_expression(
						alloc_data.get_array_length_expression(),
						scope_name,
						scope_level,
						line
					)));
				width = alloc_data.get_array_length() * alloc_data.get_subtype().get_width() + sin_widths::INT_WIDTH;
			}
			else {
				throw NonConstArrayLengthException(line);
			}
		}
		else {
			// if the length is not constant, check to see if we have a dynamic array; if not, then it's not legal
			if (alloc_data.get_qualities().is_dynamic()) {
				alloc_data.set_array_length(0);
				width = sin_widths::PTR_WIDTH;
			}
			else {
				throw CompilerException(
					"The length of a non-dynamic array must be known at compile time (use a literal or a valid constexpr)",
					compiler_errors::TYPE_VALIDITY_RULE_VIOLATION_ERROR,
					line
				);
			}
		}
    }
    else if (alloc_data.get_primary() == TUPLE) {
        for (auto it = alloc_data.get_contained_types().begin(); it != alloc_data.get_contained_types().end(); it++) {
            if (it->get_width() == 0) {
                width += get_width(*it, evaluator, structs, symbols, scope_name, scope_level, line);
            }
            else {
                width += it->get_width();
            }
        }
    }

    return width;
}
