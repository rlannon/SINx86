/*

SIN Toolchain (x86 target)
operator_expressions.cpp
Copyright 2020 Riley Lannon

Includes the implementations for some expression evaluations, namely those involving operators (unary, binary, and dot/member selection expressions)
Note that SIN does *not* support the arrow operator as syntactic sugar; you must dereference pointer structures directly

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

	// todo: update ref counts for unary operands

	std::stringstream eval_ss;

	// We need to know the data type in order to evaluate the expression properly
	DataType unary_type = expression_util::get_expression_data_type(to_evaluate.get_operand(), this->symbols, this->structs, line);

	// first, evaluate the expression we are modifying *unless* it is an ADDRESS operation
	if (to_evaluate.get_operator() != ADDRESS) {
		auto addr_p = this->evaluate_expression(to_evaluate.get_operand(), line);
		eval_ss << addr_p.first;
	}

	// switch to our operator -- only three unary operators are allowed (that don't have special expression types, such as dereferencing or address-of), but only unary minus and unary not have any effect
	switch (to_evaluate.get_operator()) {
	case exp_operator::UNARY_PLUS:
	{
		// does nothing but is allowed; generates a note stating as such
		compiler_note("Note the unary plus operator has no effect", line);
		break;
	}
	case exp_operator::UNARY_MINUS:
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
			if (unary_type.get_width() == sin_widths::DOUBLE_WIDTH) {
				eval_ss << "movsd xmm1, [" << compiler::DOUBLE_PRECISION_MASK_LABEL << "]" << std::endl;
				eval_ss << "xorpd xmm0, xmm1" << std::endl;
			}
			else {
				eval_ss << "movss xmm1, [" << compiler::SINGLE_PRECISION_MASK_LABEL << "]" << std::endl;
				eval_ss << "xorps xmm0, xmm1" << std::endl;
			}
		}
		else if (unary_type.get_primary() == INT) {
			/*

			integral types will have two's complement performed on them
			if the data is _unsigned_, then it may result in a loss of data because it will not increase the data's width

			*/

			if (unary_type.get_qualities().is_unsigned()) {
				compiler_warning(
					"Note: unary minus on unsigned data may result in data loss because the compiler will not modify the data's width",
					compiler_errors::POTENTIAL_DATA_LOSS,
					line);
			}

			// the expression is in RAX; check the width to get which register size to use
			std::string register_name = register_usage::get_register_name(RAX, unary_type);

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
			// XOR against a bitmask of 0xFF, as a boolean checks for zero or non-zero, not necessarily 1 or 0
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
		// expression does not have to be a boolean -- can be integral as well

		if (unary_type.get_primary() == INT || unary_type.get_primary() == CHAR || unary_type.get_primary() == BOOL) {
			// simply use the x86 NOT instruction
			eval_ss << "\t" << "not " << register_usage::get_register_name(RAX, unary_type) << std::endl;
		}
		else {
			throw UnaryTypeNotSupportedError(line);
		}
		break;
	}
	case exp_operator::ADDRESS:
	{
		// an address-of expression has its own function
		eval_ss << this->get_address_of(to_evaluate, RAX, line).str();
		break;
	}
	case exp_operator::DEREFERENCE:
	{
		// ensure we are dereferencing a pointer
		if (unary_type.get_primary() == PTR) {
			// the address is already in RAX, so we just need to dereference (according to the type width)
			DataType pointed_to_type = unary_type.get_subtype();	// we need to know what type the pointer points to in order to get the correct register
			std::string rax_name = get_rax_name_variant(pointed_to_type, line);
			eval_ss << "\t" << "mov " << rax_name << ", [rax]" << std::endl;
		}
		else {
			throw IllegalIndirectionException(line);
		}
		
		break;
	}
	default:
		throw IllegalUnaryOperatorError(line);
		break;
	}

	return eval_ss;
}

std::pair<std::string, size_t> compiler::evaluate_binary(Binary &to_evaluate, unsigned int line) {
	/*

	evaluate_binary
	Generates code for the evaluation of a binary expression

	The binary evaluation algorithm works as follows:
	I. Check type; if dot, proceed, else skip to II
		1. Create a member_selection object
		2. Call compiler::evaluate_member_selection
	II. If any other operator,
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

	// todo: type hinting to ensure literals don't always generate type and width mismatches when used with unsigned long/short integers

	std::stringstream eval_ss;
	size_t count = 0;

	// act based on the operator
	if (to_evaluate.get_operator() == DOT) {
		eval_ss << expression_util::evaluate_member_selection(to_evaluate, this->symbols, this->structs, RAX, line).str();
	} else {
		// get the left and right branches

		DataType left_type = expression_util::get_expression_data_type(
			to_evaluate.get_left(),
			this->symbols,
			this->structs,
			line
		);
		DataType right_type = expression_util::get_expression_data_type(
			to_evaluate.get_right(),
			this->symbols,
			this->structs,
			line
		);

		Type primary = left_type.get_primary();
		size_t data_width = left_type.get_width();
		bool is_signed = left_type.get_qualities().is_signed() || right_type.get_qualities().is_signed();

		// check for half-precision type once here instead of repeating this call multiple times in source later
		if (primary == FLOAT) {
			if (
				(left_type.get_width() == sin_widths::HALF_WIDTH) ||
				(right_type.get_primary() == FLOAT && right_type.get_width() == sin_widths::HALF_WIDTH)
			) {
				half_precision_not_supported_warning(line);
			}
		}

		// issue a warning for signed/unsigned mismatch if applicable
		if (
			(primary == INT) &&
			(left_type.get_qualities().is_signed() != right_type.get_qualities().is_signed())
		) {
			compiler_warning("Signed/unsigned mismatch", compiler_errors::SIGNED_UNSIGNED_MISMATCH, line);
		}
		
		// todo: generalize check for width mismatch warning
		// also issue a warning if the types are different widths
		if (
			(left_type.get_width() != right_type.get_width()) &&
			!(primary == STRING && right_type.get_primary() == CHAR)
		) {
			compiler_warning("Width mismatch", compiler_errors::WIDTH_MISMATCH, line);
		}

		// ensure the types are compatible before proceeding with evaluation
		if (left_type.is_compatible(right_type)) {

			// todo: ensure 16-byte stack alignment? this would allow us to use movdqa instead (and fit the System V ABI)

			// evaluate the left-hand side
			auto lhs_pair = this->evaluate_expression(to_evaluate.get_left(), line);
			eval_ss << lhs_pair.first;
			count += lhs_pair.second;

			if (left_type.get_primary() == FLOAT) {
				// "push" xmm0 ('push xmm0' is not allowed)
				eval_ss << "\t" << "sub rsp, 16" << std::endl;
				eval_ss << "\t" << "movdqu [rsp], xmm0" << std::endl;
			}
			else {
				eval_ss << "\t" << "push rax" << std::endl;	// x64 only lets us push 64-bit registers
				// don't need to adjust the compiler's offset adjustment as this will be pulled from the stack before the next statement
			}

			if (lhs_pair.second) {
				eval_ss << "; have lhs reference" << std::endl;
			}

			// evaluate the right-hand side
			auto rhs_pair = this->evaluate_expression(to_evaluate.get_right(), line);
			eval_ss << rhs_pair.first;
			count += rhs_pair.second;

			// todo: ensure dynamic returns work for ALL types

			// if the right hand side has a count, we need to slightly modify how we push
			if (rhs_pair.second) {
				// todo: this is really dumb
				eval_ss << "\t" << "pop rax" << std::endl;	// we DON'T want this if RHS is a function call
				eval_ss << "\t" << "mov r15, rax" << std::endl;
			}

			if (right_type.get_primary() == FLOAT) {
				// this depends on the data width; note that floating-point values must always convert to double if a double is used
				eval_ss << "\t" << ((right_type.get_width() == sin_widths::DOUBLE_WIDTH) ? "movsd" : "movss") << " xmm1, xmm0" << std::endl;

				// "pop" xmm0 (as 'pop xmm0' is not allowed)
				eval_ss << "\t" << "movdqu xmm0, [rsp]" << std::endl;
				eval_ss << "\t" << "add rsp, 16" << std::endl;

				// if the left type is single-precision, but right type is double, we need to convert it to double (if assigning to float, may result in loss of data); this is not considered an 'implicit conversion' by the compiler because both are floating-point types, and requisite width conversions are allowed
				if (left_type.get_width() != right_type.get_width()) {
					// if the lhs is a double, convert rhs to double; if rhs is a double, convert lhs to a double
					if (left_type.get_width() == sin_widths::DOUBLE_WIDTH) {
						eval_ss << "\t" << "cvtss2sd xmm1, xmm1" << std::endl;	// convert scalar single to scalar double, taking the value from xmm1 and storing it back in xmm1
					}
					else {
						eval_ss << "\t" << "cvtss2sd xmm0, xmm0" << std::endl;
					}

					data_width = sin_widths::DOUBLE_WIDTH;	// ensure the expression is marked as double-precision for eventual operation code generation
				}
			}
			else {
				// restore the lhs
				eval_ss << "\t" << "mov rbx, rax" << std::endl;
				
				// if we had something to free, it's the next thing on the stack
				// we want to ensure that we preserve it
				if (lhs_pair.second) {
					// todo: get safe register
					eval_ss << "\t" << "pop r12" << std::endl;
					eval_ss << "\t" << "pop rax" << std::endl;
					eval_ss << "\t" << "push r12" << std::endl;
				}
				else {
					eval_ss << "\t" << "pop rax" << std::endl;
				}
			}

			// and *now* we push the value to free
			if (rhs_pair.second) {
				eval_ss << "\t" << "push r15" << std::endl;	// again, this is very dumb
			}

			// finally, act according to the operator and type
			if (to_evaluate.get_operator() == PLUS) {
				switch (primary) {
				case INT:
				case PTR:	// pointer arithmetic with + and - is allowed in SIN
					eval_ss << "\t" << "add rax, rbx" << std::endl;
					// todo: account for sign differences between types, overflow
					break;
				case FLOAT:
					// single- and double-precision floats use different SSE instructions
					if (data_width == sin_widths::DOUBLE_WIDTH) {
						eval_ss << "\t" << "addsd xmm0, xmm1" << std::endl;	// add scalar double
					}
					else {
						eval_ss << "\t" << "addss xmm0, xmm1" << std::endl;	// add scalar single
					}
					break;
				case STRING:
				{
					/*

					string concatenation 
					
					If the right hand type is a string:
					passes pointer to string to SRE function
					
					SRE routine parameters are:
						RSI - ptr<string> left
						RDI - ptr<string> right
					Returns
						ptr<string> - points to the location of the resultant string
					
					If the right hand type is a char:
						* Increment the string length
						* Replace the null byte with the char and append a new null byte
					
					*/

					if (right_type.get_primary() == STRING) {
						eval_ss << push_used_registers(this->reg_stack.peek(), true).str();

						std::string routine_name = (right_type.get_primary() == CHAR) ? "sinl_string_append" : "sinl_string_concat";
						eval_ss << "\t" << "mov rsi, rax" << std::endl;
						eval_ss << "\t" << "mov rdi, rbx" << std::endl;

						eval_ss << call_sincall_subroutine(routine_name);
						eval_ss << pop_used_registers(this->reg_stack.peek(), true).str();
						
						count += 1;	// string concatenation and appendment allocate resources
						eval_ss << "\t" << "push rax" << std::endl;
					}
					else if (right_type.get_primary() == CHAR) {
						eval_ss << push_used_registers(this->reg_stack.peek(), true).str();

						eval_ss << "\t" << "mov rsi, rax" << std::endl;
						eval_ss << "\t" << "mov eax, [rax]" << std::endl
							<< "\t" << "mov [rsi + rax], bl" << std::endl;
						eval_ss << "\t" << "inc dword [rsi]" << std::endl;
						eval_ss << "\t" << "mov eax, [rsi]" << std::endl
							<< "\t" << "mov [rsi + rax], byte 0" << std::endl;

						eval_ss << pop_used_registers(this->reg_stack.peek(), true).str();
					}
					else {
						throw UndefinedOperatorError("concatenation", line);
					}
					
					break;
				}
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
					// todo: account for sign differences between types, overflow
					break;
				case FLOAT:
					// single- and double-precision floats use different SSE instructions
					if (data_width == sin_widths::DOUBLE_WIDTH) {
						eval_ss << "\t" << "subsd xmm0, xmm1" << std::endl;
					}
					else {
						eval_ss << "\t" << "subss xmm0, xmm1" << std::endl;
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
					auto rbx_name = register_usage::get_register_name(RBX, left_type);
					eval_ss << "\t" << "mov " << register_usage::get_register_name(RDX, left_type) << ", 0" << std::endl;
					if (is_signed) {
						eval_ss << "\t" << "imul " << rbx_name << std::endl;
					}
					else {
						eval_ss << "\t" << "mul " << rbx_name << std::endl;
					}
				}
				else if (primary == FLOAT) {
					if (data_width == sin_widths::DOUBLE_WIDTH) {
						eval_ss << "\t" << "mulsd xmm0, xmm1" << std::endl;
					}
					else {
						eval_ss << "\t" << "mulss xmm0, xmm1" << std::endl;
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
					auto rbx_name = register_usage::get_register_name(RBX, left_type);
					eval_ss << "\t" << "mov " << register_usage::get_register_name(RDX, left_type) << ", 0" << std::endl;
					if (is_signed) {
						// use idiv
						eval_ss << "\t" << "idiv " << rbx_name << std::endl;
					}
					else {
						// use div
						eval_ss << "\t" << "div " << rbx_name << std::endl;
					}
				}
				else if (primary == FLOAT) {
					// which instruction depends on the width of the values; in either case, we are operating on scalar values (not packed)
					if (data_width == sin_widths::DOUBLE_WIDTH) {
						eval_ss << "\t" << "divsd xmm0, xmm1" << std::endl;
					}
					else {
						eval_ss << "\t" << "divss xmm0, xmm1" << std::endl;
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
					auto rdx_name = register_usage::get_register_name(RDX, left_type);
					eval_ss << "\t" << "mov " << rdx_name << ", 0" << std::endl;
					eval_ss << "\t" << "div " << register_usage::get_register_name(RBX, left_type) << std::endl;
					eval_ss << "\t" << "mov " << register_usage::get_register_name(RAX, left_type) << ", " << rdx_name << std::endl;
				}
				else if (primary == FLOAT) {
					// todo: ensure we are using an xmm register that's not currently in use to hold some other data
					// the mod operator will follow the IEEE x REM y standard, yielding the result (x - (x/y)*y)
					auto fp_suffix = (data_width == sin_widths::DOUBLE_WIDTH) ? "sd" : "ss";
					eval_ss << "\t" << "mov" << fp_suffix << " xmm2, xmm0" << std::endl;
					eval_ss << "\t" << "div" << fp_suffix << " xmm0, xmm1" << std::endl;
					eval_ss << "\t" << "mul" << fp_suffix << " xmm0, xmm1" << std::endl;
					eval_ss << "\t" << "sub" << fp_suffix << " xmm2, xmm0" << std::endl;
					eval_ss << "\t" << "mov" << fp_suffix << " xmm0, xmm2" << std::endl;	// todo: verify
				}
				else {
					throw UndefinedOperatorError("modulo", line);
				}
			}

			// Bitwise operators; these may use integral types
			else if (
				(to_evaluate.get_operator() == exp_operator::BIT_AND) ||
				(to_evaluate.get_operator() == exp_operator::BIT_OR) ||
				(to_evaluate.get_operator() == exp_operator::BIT_XOR)
			) {
				std::string inst =
					(to_evaluate.get_operator() == BIT_AND) ? "and" :
					(to_evaluate.get_operator() == BIT_OR) ? "or" : "xor";
				
				if (primary == INT || primary == CHAR || primary == PTR) {
					eval_ss << "\t" << inst << " " << register_usage::get_register_name(RAX, left_type)
						<< ", " << register_usage::get_register_name(RBX, right_type) << std::endl;
				}
				else {
					throw UndefinedOperatorError("bitwise-" + inst, line);
				}
			}
			else if (
				to_evaluate.get_operator() == exp_operator::RIGHT_SHIFT ||
				to_evaluate.get_operator() == exp_operator::LEFT_SHIFT
			) {
				// get the instruction; if we have a signed value, we utilize arithmetic shifts, and if we have unsigned values, we use logical shifts
				std::string instruction;
				if (to_evaluate.get_operator() == RIGHT_SHIFT) {
					if (left_type.get_qualities().is_signed()) {
						instruction = "sar";
					}
					else {
						instruction = "shr";
					}
				}
				else {
					if (left_type.get_qualities().is_signed()) {
						instruction = "sal";
					}
					else {
						instruction = "shl";
					}
				}

				// we must utilize the CL register for shift value (or immediate)
				eval_ss << "\t" << "mov cl, bl" << std::endl;

				// bit shifts can work on integral types
				if (primary == INT || primary == PTR || primary == CHAR) {
					if (left_type.get_qualities().is_signed()) {
						compiler_warning(
							"The sign will be retained when shifting bits of a signed type",
							compiler_errors::BITSHIFT_RESULT,
							line
						);
					}
					eval_ss << "\t" << instruction << " " << register_usage::get_register_name(RAX, left_type) << ", cl" << std::endl;
				}
				else if (primary == BOOL) {
					// boolean shifts might have weird effects
					compiler_warning(
						"Bit shifting a boolean may have no effect or invert the value",
						compiler_errors::BITSHIFT_RESULT,
						line
					);
					eval_ss << "\t" << instruction << " al, cl" << std::endl;
				}
				else if (primary == FLOAT) {
					// floating point shifts might have weird effects; specify they must be integral
					throw CompilerException(
						"Bit shifting operators must utilize integral types",
						compiler_errors::UNDEFINED_OPERATOR_ERROR,
						line
					);
				}
				else {
					// undefined for this type
					throw UndefinedOperatorError("bitshift", line);
				}
			}
			// note that bitwise not is a unary operator

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

				// todo: comparisons can be optimized further, especially when context is taken into account

				// to determine whether the comparison requires unsigned operation
				bool requires_unsigned = false;
				std::string fp_suffix;	// will change the instruction based on single-/double-precision

				// how we compare is dependent on the type
				if (left_type.get_primary() == STRING) {
					// strings can only compare with = and != operators
					if (to_evaluate.get_operator() == EQUAL || to_evaluate.get_operator() == NOT_EQUAL) {
						// use the cmpsb function
						eval_ss << "\t" << "mov rsi, rax" << std::endl;
						eval_ss << "\t" << "mov rdi, rbx" << std::endl;

						// but first, check to see whether the lengths are equal
						eval_ss << "\t" << "mov eax, [rsi]" << std::endl;
						eval_ss << "\t" << "cmp eax, dword [rdi]" << std::endl;
						eval_ss << "\t" << "jne .strcmp_" << this->strcmp_num << std::endl;
						eval_ss << "\t" << "mov ecx, [rsi]" << std::endl;
						eval_ss << "\t" << "add ecx, 4" << std::endl;	// include the length information in the comparison
						eval_ss << "\t" << "repe cmpsb" << std::endl;	// this will set EFLAGS appropriately
						eval_ss << ".strcmp_" << this->strcmp_num << ":" << std::endl;
						this->strcmp_num += 1;
					}
					else {
						throw CompilerException("Illegal equivalency operator on string type", compiler_errors::UNDEFINED_OPERATOR_ERROR, line);
					}
				}
				else if (left_type.get_primary() == FLOAT) {
					// SSE has instructions for this -- use the pseudo ops
					
					if (data_width == sin_widths::DOUBLE_WIDTH) {
						fp_suffix = "sd";
					}
					else {
						fp_suffix = "ss";
					}
				}
				else {
					// if we have two unsigned variables, use unsigned comparison
					requires_unsigned = left_type.get_qualities().is_unsigned() && right_type.get_qualities().is_unsigned();
					
					// write the comparison
					eval_ss << "\t" << "cmp rax, rbx" << std::endl;
				}
				
				// a variable to hold our instruction mnemonic
				std::string instruction = "";
				std::string fp_instruction = "";

				// todo: we could write a simple utility function to get a string for the equality based on an operator (e.g., turning EQUAL into 'e' or LESS OR EQUAL to 'le'), assuming we need to use it more than once
				// todo: use seta/setb/setna/setnb/setae/setbe for unsigned comparisons (both operands unsigned)
				
				// now, switch to determine which branching instruction we need
				switch (to_evaluate.get_operator()) {
				case exp_operator::EQUAL:
					instruction = "sete";
					fp_instruction = "cmpeq";
					break;
				case exp_operator::NOT_EQUAL:
					instruction = "setne";
					fp_instruction = "cmpneq";
					break;
				case exp_operator::GREATER:
					instruction = "setg";
					// no sse instruction; must invert
					fp_instruction = "cmple";
					break;
				case exp_operator::LESS:
					instruction = "setl";
					fp_instruction = "cmplt";
					break;
				case exp_operator::GREATER_OR_EQUAL:
					instruction = "setge";
					// no sse instruction; must invert
					fp_instruction = "cmplt";
					break;
				case exp_operator::LESS_OR_EQUAL:
					instruction = "setle";
					fp_instruction = "cmple";
					break;
				default:
					// if the parser didn't catch a 'no operator', throw the exception here -- we have no more valid operators
					throw CompilerException("Undefined operator", compiler_errors::UNDEFINED_ERROR, line);
					break;
				}

				// floating-point SSE comparisons must be handled very differently than integral comparisons
				if (primary == FLOAT) {
					if (to_evaluate.get_operator() == GREATER) {
						// greater is really an inverted less or equal
						eval_ss << "\t" << fp_instruction << fp_suffix << " xmm1, xmm0" << std::endl;
						eval_ss << "\t" << "mov" << fp_suffix << " xmm0, xmm1" << std::endl;
					}
					else if (to_evaluate.get_operator() == GREATER_OR_EQUAL) {
						// greater or equal is really an inverted less than
						eval_ss << "\t" << fp_instruction << fp_suffix << " xmm1, xmm0" << std::endl;
						eval_ss << "\t" << "mov" << fp_suffix << " xmm0, xmm1" << std::endl;
					}
					else {
						// all other comparisons can be performed normally
						eval_ss << "\t" << fp_instruction << fp_suffix << " xmm0, xmm1" << std::endl;
					}

					// now, xmm0 contains the mask -- 0xffffffff if the result was 'true', else 0x0
					eval_ss << "\t" << "sub rsp, " << ((data_width == sin_widths::DOUBLE_WIDTH) ? 8 : 4) << std::endl;
					eval_ss << "\t" << "mov" << fp_suffix << " [rsp], xmm0" << std::endl;
					eval_ss << "\t" << "mov " << ((data_width == sin_widths::DOUBLE_WIDTH) ? "rax" : "eax") << ", [rsp]" << std::endl;
					eval_ss << "\t" << "and rax, 1" << std::endl;
					eval_ss << "\t" << "add rsp, " << ((data_width == sin_widths::DOUBLE_WIDTH) ? 8 : 4) << std::endl;
				}
				else {
					// set al accordingly
					eval_ss << "\t" << instruction << " al" << std::endl;
				}
			}
		}
		else {
			// if the types were not compatible, throw a type error
			throw TypeException(line);
		}
	}

	// finally, return the generated code
	return std::make_pair<>(eval_ss.str(), count);
}
