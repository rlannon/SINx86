/*

SIN Toolchain (x86 target)
operator_expressions.cpp
Copyright 2020 Riley Lannon

Includes the implementations for some expression evaluations, namely those involving operators

*/

#include "compiler.h"

std::stringstream compiler::evaluate_unary(Unary &to_evaluate, unsigned int line) {
	/*

	evaluate_unary
	Generates code to evaluate a unary expression

	@param  to_evaluate The unary expression we are evaluating
	@param  line    The line number where the expression occurs
	@return A stringstream containing the generated code

	*/

	std::stringstream eval_ss;

	// We need to know the data type in order to evaluate the expression properly
	DataType unary_type = get_expression_data_type(to_evaluate.get_operand(), this->symbol_table, line);

	// first, evaluate the expression we are modifying
	eval_ss << this->evaluate_expression(to_evaluate.get_operand(), line).str();

	// switch to our operator -- only three unary operators are allowed (that don't have special expression types, such as dereferencing or address-of), but only unary minus and unary not have any effect
	switch (to_evaluate.get_operator()) {
	case exp_operator::PLUS:
	{
		// does nothing but is allowed; generates a warning stating as such
		compiler_warning("While the unary plus operator is technically allowed, it does nothing", line);
		break;
	}
	case exp_operator::MINUS:
	{
		// the unary minus operator may only be used on integral and floating-point types
		// this flips the sign on floats and performs two's complement on integers

		if (unary_type.get_primary() == FLOAT) {
			/*

			floating-point types also reverse the sign bit, accomplished through the use of the fchs instruction
			unlike with integers, this will not result in data loss

			the instruction should generate the following code:
				movss/movsd xmm1, [sinl_sp_mask/sinl_dp_mask]
				xorps/xorpd xmm0, xmm1

			*/

			// the floating-point expression to negate will already be in the XMM0 register; act based on width
			if (unary_type.get_width() == sin_widths::FLOAT_WIDTH) {
				eval_ss << "movss xmm1, [sinl_sp_mask]" << std::endl;
				eval_ss << "xorps xmm0, xmm1" << std::endl;
			}
			else if (unary_type.get_width() == sin_widths::DOUBLE_WIDTH) {
				eval_ss << "movsd xmm1, [sinl_dp_mask]" << std::endl;
				eval_ss << "xorpd xmm0, xmm1" << std::endl;
			}
			else {
				// todo: width exception? or should 'half' type just be converted to single-precision automatically and generate a warning?
			}
		}
		else if (unary_type.get_primary() == INT) {
			/*

			integral types will have two's complement performed on them
			if the data is _unsigned_, then it may result in a loss of data because it will not increase the data's width

			*/

			if (unary_type.get_qualities().is_unsigned()) {
				compiler_warning("Note: unary minus on unsigned data may result in a loss of data because the compiler will not increase the data's width", line);
			}

			// the expression is in RAX; check the width to get which register size to use
			std::string register_name;
			if (unary_type.get_width() == sin_widths::SHORT_WIDTH) {
				register_name = "ax";
			}
			else if (unary_type.get_width() == sin_widths::INT_WIDTH) {
				register_name = "eax";
			}
			else {
				register_name = "rax";
			}

			// perform two's complement on A with the 'neg' instruction
			eval_ss << "\t" << "neg " << register_name << std::endl;
		}
		else {
			throw UnaryTypeNotSupportedError(line);
		}

		break;
	}
	case exp_operator::NOT:
	{
		// expression must be a boolean

		if (unary_type.get_primary() == BOOL) {
			// XOR against a bitmask of 0xFF, as a boolean checks for zero or non-zero, not 1 or 0
			// a boolean will be in al
			eval_ss << "\t" << "mov ah, 0xFF" << std::endl;
			eval_ss << "\t" << "xor al, ah" << std::endl;
		}
		else {
			throw UnaryTypeNotSupportedError(line);
		}

		break;
	}
	case exp_operator::BIT_NOT:
	{
		// expression does not have to be a boolean, it can be any fixed-width type in RAX

		if (unary_type.get_primary() != STRING && unary_type.get_primary() != ARRAY && unary_type.get_primary() != STRUCT) {
			// simply use the x86 NOT instruction
			eval_ss << "\t" << "not rax" << std::endl;
		}
		else {
			throw UnaryTypeNotSupportedError(line);
		}
	}
	default:
		throw IllegalUnaryOperatorError(line);
		break;
	}

	return eval_ss;
}

std::stringstream compiler::evaluate_binary(Binary &to_evaluate, unsigned int line) {
	/*

	evaluate_binary
	Generates code for the evaluation of a binary expression

	The binary evaluation algorithm works as follows:
		1. Look at the left operand
			A. Call evaluate_expression on it; this may end up recursively evaluating binary trees
			B. Push necessary register values to the stack to preserve them
		2. Now look at the right operand
			A. Call evaluate_expression on it; again, this may end up recursively evaluating binary trees
			B. Move the result out of RBX or XMM1
			C. Pull the previously pushed values into RAX or XMM0
		3. Generate code for the current expression
			If the value is being returned from a recursive call, the result will be pushed to the stack or moved in registers when necessary

	Note that every time the stack is used, this->stack_offset and max_offset must be adjusted so that the values can be retrieved, pushed, pulled, and stored reliably. If stack_offset and max_offset are not adjusted, errors can and _will_ occur when working with the stack

	For example: looking at the expresion 3 * 4 + 5:
		1. Look at the left operand: binary expression 3 * 4
			A. Call evaluate_expression: sends us here
				1. Look at the left hand expression: 3
				2. Look at the right hand expression: 4
				3. Generate code: mul eax, ebx
			B. Push result (rax) to stack
		2. Look at the right operand: literal 5
			A. Evaluate: 5
			B. Move to ebx
			C. Pull value back into eax
		3. Generate code: add eax, ebx

	*/

	std::stringstream eval_ss;

	// get the left and right branches
	DataType left_type = get_expression_data_type(to_evaluate.get_left(), this->symbol_table, line);
	DataType right_type = get_expression_data_type(to_evaluate.get_right(), this->symbol_table, line);

	Type primary = left_type.get_primary();
	size_t data_width = left_type.get_width();
	bool is_signed = left_type.get_qualities().is_signed() || right_type.get_qualities().is_signed();

	// ensure the types are compatible before proceeding with evaluation
	if (left_type.is_compatible(right_type)) {

		// evaluate the left-hand side
		eval_ss << this->evaluate_expression(to_evaluate.get_left(), line).str();
		if (left_type.get_primary() == FLOAT) {
			// todo: preserve xmm0
		}
		else {
			eval_ss << "\t" << "push rax" << std::endl;	// x64 only lets us push 64-bit registers
			// don't need to adjust the compiler's offset adjustment as this will be pulled from the stack before the next statement
		}

		// evaluate the right-hand side
		eval_ss << this->evaluate_expression(to_evaluate.get_right(), line).str();
		if (right_type.get_primary() == FLOAT) {
			// this depends on the data width; note that floating-point values must always convert to double if a double is used
			eval_ss << "\t" << ((right_type.get_width() == sin_widths::DOUBLE_WIDTH) ? "movsd" : "movss") << " xmm1, xmm0" << std::endl;

			// todo: restore xmm0 and the stack pointer

			// if the left type is single-precision, but right type is double, we need to convert it to double (if assigning to float, may result in loss of data); this is not considered an 'implicit conversion' by the compiler because both are floating-point types, and requisite width conversions are allowed
			if (left_type.get_width() != right_type.get_width()) {
				if (left_type.get_width() == sin_widths::FLOAT_WIDTH) {
					eval_ss << "\t" << "cvtss2sd xmm0, xmm0" << std::endl;	// todo: is this a valid instruction?
					data_width = sin_widths::DOUBLE_WIDTH;	// ensure the expression is marked as double-precision for eventual operation code generation
				}
				else {
					eval_ss << "\t" << "cvtss2sd xmm1, xmm1" << std::endl;	// convert scalar single to scalar double, taking the value from xmm1 and storing it back in xmm1
				}
			}
		}
		else {
			eval_ss << "\t" << "mov rbx, rax" << std::endl;
			eval_ss << "\t" << "pop rax" << std::endl;
		}

		// finally, act according to the operator and type
		if (to_evaluate.get_operator() == PLUS) {
			switch (primary) {
			case INT:
			case PTR:	// pointer arithmetic with + and - is allowed in SIN
				eval_ss << "\t" << "add rax, rbx" << std::endl;
				break;
			case FLOAT:
				// single- and double-precision floats use different SSE instructions
				if (data_width == sin_widths::FLOAT_WIDTH) {
					eval_ss << "\t" << "addss xmm0, xmm1" << std::endl;	// add scalar single
				}
				else {
					eval_ss << "\t" << "addsd xmm0, xmm1" << std::endl;	// add scalar double
				}
				break;
			case STRING:
				// todo: string concatenation (passes pointer to string)
				break;
			default:
				// if we have an invalid type, throw an exception
				// todo: should array concatenation be allowed with the + operator?
				throw UndefinedOperatorError("plus", line);
				break;
			}
		}
		else if (to_evaluate.get_operator() == MINUS) {
			switch (primary) {
			case INT:
			case PTR:
				eval_ss << "\t" << "sub rax, rbx" << std::endl;
				break;
			case FLOAT:
				// single- and double-precision floats use different SSE instructions
				if (data_width == sin_widths::FLOAT_WIDTH) {
					eval_ss << "\t" << "subss xmm0, xmm1" << std::endl;
				}
				else {
					eval_ss << "\t" << "subsd xmm0, xmm1" << std::endl;
				}
				break;
			default:
				// the minus operator is undefined for all other types
				throw UndefinedOperatorError("minus", line);
				break;
			}
		}
		else if (to_evaluate.get_operator() == MULT) {
			// mult only allowed for int and float
			if (primary == INT) {
				// we have to decide between mul and imul instructions -- use imul if either of the operands is signed
				if (is_signed) {
					eval_ss << "\t" << "imul rax, rbx" << std::endl;
				}
				else {
					eval_ss << "\t" << "mul rax, rbx" << std::endl;
				}
			}
			else if (primary == FLOAT) {
				if (data_width == sin_widths::FLOAT_WIDTH) {
					eval_ss << "\t" << "mulss xmm0, xmm1" << std::endl;
				}
				else {
					eval_ss << "\t" << "mulsd xmm0, xmm1" << std::endl;
				}
			}
			else {
				// todo: throw exception
				throw UndefinedOperatorError("multiplication", line);
			}
		}
		else if (to_evaluate.get_operator() == DIV)
		{
			// div only allowed for int and float
			if (primary == INT) {
				// how we handle integer division depends on whether we are using signed or unsigned integers
				if (is_signed) {
					// use idiv
					eval_ss << "\t" << "idiv rax, rbx" << std::endl;
				}
				else {
					// use div
					eval_ss << "\t" << "div rax, rbx" << std::endl;
				}
			}
			else if (primary == FLOAT) {
				// which instruction depends on the width of the values; in either case, we are operating on scalar values (not packed)
				if (data_width == sin_widths::FLOAT_WIDTH) {
					eval_ss << "\t" << "divss xmm0, xmm1" << std::endl;
				}
				else {
					eval_ss << "\t" << "divsd xmm0, xmm1" << std::endl;
				}
			}
			else {
				throw UndefinedOperatorError("division", line);
			}
		}
		else if (to_evaluate.get_operator() == MODULO)
		{
			// modulo only allowed for int and float
			if (primary == INT) {
				// for modulo, we need to determine what should happen if we are using signed numbers
				// todo: modulo
			}
			else if (primary == FLOAT) {
				// todo: implement modulo with floating-point numbers
			}
			else {
				throw UndefinedOperatorError("modulo", line);
			}
		}

		// Bitwise operators; these may use int or float
		else if (to_evaluate.get_operator() == exp_operator::BIT_AND)
		{
			if (primary == INT) {
				// doesn't matter whether we have signed or unsigned data, but we should issue a warning for types of differing widths
				if (left_type.get_width() != right_type.get_width()) {
					compiler_warning("Operands in bitwise operation are different widths", line);
				}

				eval_ss << "\t" << "and rax, rbx" << std::endl;
			}
			else if (primary == FLOAT) {
				// todo: floats with bitwise operators
			}
			else {
				throw UndefinedOperatorError("bitwise-and", line);
			}
		}
		else if (to_evaluate.get_operator() == exp_operator::BIT_OR)
		{
			// same procedure as bitwise-and
			if (primary == INT) {
				// doesn't matter whether we have signed or unsigned data, but we should issue a warning for types of differing widths
				if (left_type.get_width() != right_type.get_width()) {
					compiler_warning("Operands in bitwise operation are different widths", line);
				}

				eval_ss << "\t" << "or rax, rbx" << std::endl;
			}
			else if (primary == FLOAT) {
				// todo: floats with bitwise operators
			}
			else {
				throw UndefinedOperatorError("bitwise-or", line);
			}
		}
		else if (to_evaluate.get_operator() == exp_operator::BIT_XOR)
		{
			// bitwise xor
			if (primary == INT) {
				// doesn't matter whether we have signed or unsigned data, but we should issue a warning for types of differing widths
				if (left_type.get_width() != right_type.get_width()) {
					compiler_warning("Operands in bitwise operation are different widths", line);
				}

				eval_ss << "\t" << "xor rax, rbx" << std::endl;
			}
			else if (left_type.get_primary() == FLOAT) {
				// todo: floats with bitwise operators
			}
			else {
				throw UndefinedOperatorError("bitwise-xor", line);
			}
		}
		// bitwise not is a unary operator

		/*

		Logical operators

		These may only operate on boolean types
		They generate identical code to their bitwise counterparts but different errors

		*/
		else if (to_evaluate.get_operator() == exp_operator::AND)
		{
			// logical and
			if (primary == BOOL) {
				eval_ss << "\t" << "and al, bl" << std::endl;
			}
			else {
				throw UndefinedOperatorError("logical-and", line);
			}
		}
		else if (to_evaluate.get_operator() == exp_operator::OR)
		{
			// logical or
			if (primary == BOOL) {
				eval_ss << "\t" << "or al, bl" << std::endl;
			}
			else {
				throw UndefinedOperatorError("logical-or", line);
			}
		}
		else if (to_evaluate.get_operator() == exp_operator::XOR)
		{
			// logical xor
			if (primary == BOOL) {
				eval_ss << "\t" << "xor al, bl" << std::endl;
			}
			else {
				throw UndefinedOperatorError("logical-xor", line);
			}
		}
		// logical not is a unary operator

		// since we have tested all other valid operators, it must be an equivalency operator
		else {
			/*

			Equivalency operators may be used on all types

			The equivalency operators will use the CMP instruction on RAX and RBX to test for (in)equality, then use the SETcc instruction to set RAX
			This will result in 1 in AL if the condition was true, or 0 if it was false; we then use MOVZX to extend AL to RAX, ensuring we don't have any garbage data remaining in higher bits in the register

			*/

			// write the comparison
			eval_ss << "\t" << "cmp rax, rbx" << std::endl;

			// a variable to hold our instruction mnemonic
			std::string instruction = "";

			// todo: we could write a simple utility function to get a string for the equality based on an operator (e.g., turning EQUAL into 'e' or LESS OR EQUAL to 'le'), assuming we need to use it more than once

			// now, switch to determine which branching instruction we need
			switch (to_evaluate.get_operator()) {
			case exp_operator::EQUAL:
				instruction = "sete";
				break;
			case exp_operator::NOT_EQUAL:
				instruction = "setne";
				break;
			case exp_operator::GREATER:
				instruction = "setg";
				break;
			case exp_operator::LESS:
				instruction = "setl";
				break;
			case exp_operator::GREATER_OR_EQUAL:
				instruction = "setge";
				break;
			case exp_operator::LESS_OR_EQUAL:
				instruction = "setle";
				break;
			default:
				// if the parser didn't catch a 'no operator', throw the exception here -- we have no more valid operators
				throw CompilerException("Undefined operator", compiler_errors::UNDEFINED_ERROR, line);
				break;
			}

			// write the instruction sequence
			eval_ss << "\t" << instruction << " al" << std::endl;
			eval_ss << "\t" << "movzx rax, al" << std::endl;
		}
	}
	else {
		// if the types were not compatible, throw a type error
		throw TypeException(line);
	}

	// finally, return the generated code
	return eval_ss;
}