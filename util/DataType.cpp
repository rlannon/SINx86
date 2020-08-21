/*

SIN Toolchain
DataType.cpp
Copyright 2019 Riley Lannon

Implementation of the DataType class

*/

#include "DataType.h"

void DataType::set_width() {
	// Sets the width of the type based on its primary type and symbol qualities
	
	// The width of all dynamic memory is PTR_WIDTH
	if (this->qualities.is_dynamic()) {
		this->width = sin_widths::PTR_WIDTH;
	}
	else {
		// All other types have different widths
		if (this->primary == INT) {
			// ints are usually 4 bytes wide (32-bit), but can be 2 bytes for a short or 8 for a long 

			if (this->qualities.is_long()) {
				this->width = sin_widths::LONG_WIDTH;
			} else if (this->qualities.is_short()) {
				this->width = sin_widths::SHORT_WIDTH;
			} else {
				this->width = sin_widths::INT_WIDTH;
			}
		} else if (this->primary == FLOAT) {
			// floats can also use the long and short keywords -- a long float is the same as a double, a short float is the same as a half

			if (this->qualities.is_long()) {
				this->width = sin_widths::DOUBLE_WIDTH;
			} else if (this->qualities.is_short()) {
				this->width = sin_widths::HALF_WIDTH;	// line number information, for the time being, will be caught wherever this type is used
				compiler_warning(
					"16-bit half-precision floats are not supported by the SIN compiler at this time; using 32-bit single-precision instead",
					compiler_errors::DATA_WIDTH_ERROR,
					0	// todo: line number information
				);
			} else {
				this->width = sin_widths::FLOAT_WIDTH;
			}
		} else if (this->primary == BOOL) {
			// bools are only a byte wide
			this->width = sin_widths::BOOL_WIDTH;
		} else if (this->primary == PTR || this->primary == REFERENCE) {
			// because we are compiling to x86_64, pointers and references should be 64-bit
			this->width = sin_widths::PTR_WIDTH;
		} else if (this->primary == STRING) {
			// since strings are all implemented as pointers, they have widths of 8 bytes
			// (they always point to dynamic memory, but syntactically don't behave like pointers)
			this->width = sin_widths::PTR_WIDTH;
		} else if (this->primary == CHAR) {
			// todo: determine whether it is ASCII or UTF-8
			this->width = sin_widths::CHAR_WIDTH;
		} else if (this->primary == TUPLE) {
			// tuple widths are known at compile time -- it is the sum of the widths of each of their contained types
			// however, if they contain a struct or an array, we must defer the width evaluation until we have a struct table
			this->width = 0;
			auto it = this->contained_types.begin();
			bool defer_width = false;
			while (it != this->contained_types.end() && !defer_width) {
				if (it->get_width() == 0) {	// types with unknown lengths have a width of 0
					this->width = 0;
					defer_width = true;
				}
				else {
					this->width += it->get_width();
				}

				it++;
			}
		}
		else {
			/*

			Everything else should use 0:
				void	-	a void type is literally nothing
				array	-	do not have defined widths, it depends on the array and its subtype
				struct	-	require the compiler to look for the width in the struct table
			
			While it is possible to calculate the width of arrays and structs if all of that information is known at compile time, it is possible that a struct member would only be known to the compiler through the "decl" keyword, in which case it would be impossible to know the width when the allocation was occurring.

			*/

			this->width = 0;
		}
	}
}

/*

We can use the overloaded operators to do any of the following:
	- See if two DataType objects are equal
	- See if DataType is equal to {Type, Type}
	- See if DataType is equal to Type

*/

DataType& DataType::operator=(const DataType &right)
{
	// Move assignment operator
	if (this != &right) {
		this->primary = right.primary;
		this->contained_types = right.contained_types;
		this->qualities = right.qualities;
		this->array_length = right.array_length;
		this->struct_name = right.struct_name;
		this->width = right.width;
		this->array_length_expression = right.array_length_expression;
	}

	return *this;
}

bool DataType::operator==(const DataType& right) const
{
	bool primary_match = this->primary == right.primary;
	bool subtype_match = true;
	if (!this->contained_types.empty() && !right.contained_types.empty()) {
		subtype_match = this->contained_types == right.contained_types;
	}
	else
		subtype_match = this->contained_types == right.contained_types;
	bool qualities_match = this->qualities == right.qualities;

	return primary_match && subtype_match && qualities_match;
}

bool DataType::operator!=(const DataType& right) const
{
	return !this->operator==(right);
}

bool DataType::operator==(const Type right)
{
	return this->primary == right;
}

bool DataType::operator!=(const Type right)
{
	return this->primary != right;
}

const bool DataType::is_valid_type_promotion(symbol_qualities left, symbol_qualities right) {
	/*
	
	is_valid_type_promotion
	Ensures that type promotion rules are not broken

	Essentially, the right-hand variability quality must be equal to or *lower* than the left-hand one in the hierarchy

	See doc/Type Compatibility.md for information on type promotion
	
	*/

	if (left.is_const()) {
		return true;	// a const left-hand side will always promote the right-hand expression
	}
	else if (left.is_final()) {
		return !right.is_const();	// a const right-hand argument cannot be demoted
	}
	else {
		return !(right.is_const() || right.is_final());	// the right-hand argument cannot be demoted
	}
}

bool DataType::is_compatible(DataType to_compare) const
{
	/*
	
	Compares 'this' with 'to_compare' as if 'this' was the left-hand operand and 'to_compare' was the right-hand.
	Types are compatible if one of the following is true:
		- if pointer or array type:
			- subtypes are compatible
			- one of the subtypes is RAW
		- else,
			- left OR right is RAW
			- if reference:
				- the subtype is compatible with to_compare
			- primaries are equal
			- primaries are string and char

	*/

	bool compatible = false;

	if (this->primary == RAW || to_compare.get_primary() == RAW) {
		compatible = true;
	}
	else if (this->primary == PTR && to_compare.get_primary() == PTR) {
		// call is_compatible on the subtypes and ensure the type promotion is legal
		if (!this->contained_types.empty() && !to_compare.contained_types.empty()) {
			compatible = this->get_subtype().is_compatible(
				to_compare.get_subtype()
			) && is_valid_type_promotion(this->get_subtype().qualities, to_compare.get_subtype().qualities);
		} else {
			throw CompilerException("Expected subtype", 0, 0);	// todo: ptr and array should _always_ have subtypes
		}
	}
	else if (this->primary == REFERENCE) {
		// if we have a reference type, compare the reference subtype to to_compare
		if (!this->contained_types.empty()) {
			compatible = this->get_subtype().is_compatible(to_compare);
		}
		else {
			throw CompilerException("Expected subtype", 0, 0);
		}
	}
	else if (this->primary == ARRAY && to_compare.get_primary() == ARRAY) {
		if (!this->contained_types.empty()) {
			compatible = this->get_subtype().is_compatible(
				to_compare.get_subtype()
			);
		}
		else {
			throw CompilerException("Expected subtype", 0, 0);
		}
	}
	else if (this->primary == TUPLE && to_compare.get_primary() == TUPLE) {
		// tuples must have the same number of elements, and in the same order, to be compatible
		if (this->contained_types.size() == to_compare.contained_types.size()) {
			compatible = true;
			auto this_it = this->contained_types.begin();
			auto comp_it = to_compare.contained_types.begin();
			while (compatible && (this_it != this->contained_types.end()) && (comp_it != to_compare.contained_types.end())) {
				if (this_it->is_compatible(*comp_it)) {
					this_it++;
					comp_it++;
				}
				else {
					compatible = false;
				}
			}
		}
	}
	else {
		// primary types must be equal
		// todo: generate warnings for width and sign differences
		compatible = (
			(this->primary == to_compare.primary) || 
			(this->primary == STRING && to_compare.primary == CHAR)
		);
	}

	return compatible;
}

Type DataType::get_primary() const
{
	return this->primary;
}

symbol_qualities DataType::get_qualities() const {
	return this->qualities;
}

size_t DataType::get_array_length() const {
	return this->array_length;
}

std::string DataType::get_struct_name() const {
	return this->struct_name;
}

std::shared_ptr<Expression> DataType::get_array_length_expression() const {
	return this->array_length_expression;
}

DataType DataType::get_subtype() const {
	DataType to_return(NONE);
	
	if (!this->contained_types.empty()) {
		to_return = this->contained_types[0];
	}

	return to_return;
}

std::vector<DataType> &DataType::get_contained_types() {
	return this->contained_types;
}

bool DataType::has_subtype() const {
	return !this->contained_types.empty();
}

void DataType::set_primary(Type new_primary) {
	this->primary = new_primary;
}

void DataType::set_subtype(DataType new_subtype) {
	if (!this->contained_types.empty()) {
		this->contained_types[0] = new_subtype;
	}
	else {
		this->contained_types.push_back(new_subtype);
	}
}

void DataType::set_contained_types(std::vector<DataType> types_list) {
	this->contained_types = types_list;
}

void DataType::set_array_length(size_t new_length) {
	this->array_length = new_length;
}

void DataType::add_qualities(symbol_qualities to_add) {
	// simply use the "SymbolQualities::add_qualities" function
	this->qualities.add_qualities(to_add);

    // update the width
    this->set_width();
}

void DataType::add_quality(SymbolQuality to_add) {
    // add a quality 'to_add' to the data type
    this->qualities.add_quality(to_add);

	// generate a compiler warning if the primary type doesn't support the quality (has no effect)
	if (this->primary == PTR || this->primary == BOOL || this->primary == ARRAY || this->primary == STRING || this->primary == RAW) {
		if (to_add == LONG || to_add == SHORT || to_add == SIGNED || to_add == UNSIGNED) {
			compiler_note("Width and sign qualifiers have no effect for this type; as such, this quality will be ignored");
		}
	}

    // update the width
    this->set_width();
}

void DataType::set_struct_name(std::string name) {
	// Sets the name of the struct
	this->struct_name = name;
}

size_t DataType::get_width() const {
	return this->width;
}

bool DataType::is_valid_type(DataType &t) {
	/*
	
	is_valid_type
	Checks to ensure the DataType object follows all of SIN's type rules
	
	*/

	bool is_valid = true;

	if (t.primary == ARRAY) {
		// the array length must not be 0 if the array is not dynamic
		if (t.array_length == 0 && !t.qualities.is_dynamic()) {
			is_valid = false;
		}
	}
	else if (t.primary == FLOAT) {
		// half-precision or short floats are not supported
		if (t.qualities.is_short()) {
			is_valid = false;
		}
	}
	else if (t.primary == STRING) {
		// strings are not numerics and so may not be used with signed or unsigned qualifiers
		if (t.qualities.is_signed() || t.qualities.is_unsigned()) {
			is_valid = false;
		}

		// they may also not use 'static' unless they are 'static const'; they are inherently dynamic
		if (t.qualities.is_static() && !t.get_qualities().is_const()) {
			is_valid = false;
		}
	}
	else if (t.primary == STRUCT) {
		// structs don't support numeric or width qualifiers
		if (t.qualities.is_long() || t.qualities.is_short() || t.qualities.is_signed() || t.qualities.is_unsigned()) {
			is_valid = false;
		}
	}

	// todo: more type checks where needed

	return is_valid;
}

bool DataType::is_reference_type() const {
	/*

	is_reference_type
	Returns whether the type in question is a reference type

	*/

	return this->get_qualities().is_dynamic() || this->primary == STRING || this->primary == REFERENCE;
}

bool DataType::must_initialize() const {
	/*

	must_initialize
	Determines whether the type must be initialized in its allocation

	Check the documentation (specifically, docs/Construction.md) for the exact rules

	*/

	bool init_required = false;
	init_required = this->get_qualities().is_const();
	init_required = init_required || this->get_primary() == REFERENCE;
	if (this->get_primary() == ARRAY) {

	}
	
	// todo: how to handle tuples?
	return init_required;
}

DataType::DataType(Type primary, DataType subtype, symbol_qualities qualities, std::shared_ptr<Expression> array_length_exp, std::string struct_name) :
    primary(primary),
    qualities(qualities),
	array_length_expression(array_length_exp),
	struct_name(struct_name)
{
	// create the vector with our subtype
	// if we have a string type, set the subtype to CHAR
	if (primary == STRING) {
		subtype = DataType(CHAR);
	}

	this->contained_types = { subtype };

	// the array length will be evaluated by the compiler; start at 0
	this->array_length = 0;
	
    // if the type is int, set signed to true if it is not unsigned
	if (primary == INT && !this->qualities.is_unsigned()) {
		this->qualities.add_quality(SIGNED);
	}
	else if (primary == FLOAT)
	{
		this->qualities.add_quality(SIGNED);
	}

	// set the data width
	this->set_width();
}

DataType::DataType(Type primary, std::vector<DataType> contained_types, symbol_qualities qualities):
	primary(primary),
	contained_types(contained_types),
	qualities(qualities)
{
	// update the rest of our members
	this->array_length = 0;
	this->struct_name = "";
	this->set_width();
}

DataType::DataType(Type primary) :
	DataType(
		primary,
		DataType(),
		symbol_qualities(),
		nullptr,
		""
	)
{
	// The purpose of this constructor is to allow a Type to be converted into a DataType object
	// no body needed (super called)
}

DataType::DataType(const DataType &ref) {
	this->primary = ref.primary;
	this->contained_types = ref.contained_types;
	this->qualities = ref.qualities;
	this->array_length = ref.array_length;
	this->array_length_expression = ref.array_length_expression;
	this->struct_name = ref.struct_name;
	this->width = ref.width;
}

DataType::DataType()
{
	this->primary = NONE;
	this->qualities = symbol_qualities();	// no qualities to start
	this->array_length = 0;
	this->width = 0;
	this->struct_name = "";
	this->array_length_expression = nullptr;
}

DataType::~DataType()
{
}
