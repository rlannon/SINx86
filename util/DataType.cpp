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
	} else {
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
				this->width = sin_widths::HALF_WIDTH;
			} else {
				this->width = sin_widths::FLOAT_WIDTH;
			}
		} else if (this->primary == BOOL) {
			// bools are only a byte wide
			this->width = sin_widths::BOOL_WIDTH;
		} else if (this->primary == PTR) {
			// because we are compiling to x86_64, pointers should be 64-bit
			this->width = sin_widths::PTR_WIDTH;
		} else if (this->primary == STRING) {
			// since strings are all implemented as pointers, they have widths of 8 bytes
			// (they always point to dynamic memory, but syntactically don't behave like pointers)
			this->width = sin_widths::PTR_WIDTH;
		} else {
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
		this->subtype = right.subtype;
		this->qualities = right.qualities;
		this->array_length = right.array_length;
		this->struct_name = right.struct_name;
		this->width = right.width;
	}

	return *this;
}

bool DataType::operator==(const DataType right)
{
	return (this->primary == right.primary) && (this->subtype == right.subtype);
}

bool DataType::operator!=(const DataType right)
{
	return (this->primary != right.primary) || (this->subtype != right.subtype);
}

bool DataType::operator==(const Type right[2])
{
	return (this->primary == right[0]) && (*this->subtype == right[1]);
}

bool DataType::operator!=(const Type right[2])
{
	return (this->primary != right[0]) || (*this->subtype != right[1]);
}

bool DataType::operator==(const Type right)
{
	return this->primary == right;
}

bool DataType::operator!=(const Type right)
{
	return this->primary != right;
}

bool DataType::is_compatible(DataType to_compare) const
{
	/*
	
	Compares 'this' with 'to_compare' as if 'this' was the left-hand operand and 'to_compare' was the right-hand.
	Types are compatible if one of the following is true:
		- if pointer or array type:
			- subtypes are compatible
			- one of the subtypes is RAW
		- left OR right is RAW
		- primaries are equal

	*/

	if (this->primary == RAW || to_compare.get_primary() == RAW) {
		return true;
	}
	else if ((this->primary == PTR && to_compare.get_primary() == PTR) || (this->primary == ARRAY && to_compare.get_primary() == ARRAY))
	{
		// cast the subtypes to DataType (with a subtype of NONE) and call is_compatible on them
		if (this->subtype && to_compare.subtype) {
			return this->subtype->is_compatible(
				*dynamic_cast<DataType*>(to_compare.get_full_subtype().get())
			);
		} else {
			throw CompilerException("Expected subtype", 0, 0);	// todo: ptr and array should _always_ have subtypes
		}
	}
	else {
		// primary types must be equal
		// todo: generate warnings for width and sign differences
		return this->primary == to_compare.primary;
	}
}

Type DataType::get_primary() const
{
	return this->primary;
}

Type DataType::get_subtype() const
{
	if (this->subtype) {
		return this->subtype->get_primary();
	} else {
		return NONE;
	}
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

std::shared_ptr<DataType> DataType::get_full_subtype() const {
	// static_cast to a subtype does not work; we need a function to return the entire shared pointer
	return this->subtype;
}

void DataType::set_primary(Type new_primary) {
	this->primary = new_primary;
}

void DataType::set_subtype(DataType new_subtype) {
	this->subtype = std::make_shared<DataType>(new_subtype);
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
			compiler_warning("Width and sign qualifiers have no effect for this type; as such, this quality will be ignored");
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

	if (t.primary == FLOAT) {
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

DataType::DataType(Type primary, DataType subtype, symbol_qualities qualities, size_t array_length, std::string struct_name) :
    primary(primary),
	subtype(nullptr),
    qualities(qualities),
	array_length(array_length),
	struct_name(struct_name)
{
	// if the subtype has a type of NONE, then the subtype should be a nullptr; otherwise, construct an object
	if (subtype.get_primary() != NONE) {
		this->subtype = std::make_shared<DataType>(subtype);
	}

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

DataType::DataType(Type primary) :
	DataType(
		primary,
		DataType(),
		symbol_qualities(),
		0,
		""
	)
{
	// The purpose of this constructor is to allow a Type to be converted into a DataType object
	// no body needed (super called)
}

DataType::DataType(const DataType &ref) {
	this->primary = ref.primary;
	this->subtype = ref.subtype;
	this->qualities = ref.qualities;
	this->array_length = ref.array_length;
	this->struct_name = ref.struct_name;
	this->width = ref.width;
}

DataType::DataType()
{
	this->primary = NONE;
	this->subtype = nullptr;
	this->qualities = symbol_qualities();	// no qualities to start
	this->array_length = 0;
	this->struct_name = "";
}

DataType::~DataType()
{
	this->subtype = nullptr;
}
