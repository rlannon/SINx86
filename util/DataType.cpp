/*

SIN Toolchain
DataType.cpp
Copyright 2019 Riley Lannon

Implementation of the DataType class

*/

#include "DataType.h"

/*

SymbolQualities implementation

*/

const std::unordered_map<std::string, SymbolQuality> SymbolQualities::quality_strings = {
	{ "const", CONSTANT },
	{ "final", FINAL },
	{ "static", STATIC },
	{ "dynamic", DYNAMIC },
	{ "long", LONG },
	{ "short", SHORT },
	{ "signed", SIGNED },
	{ "unsigned", UNSIGNED }
};

bool SymbolQualities::is_long()
{
    return long_q;
}

bool SymbolQualities::is_short()
{
    return short_q;
}

bool SymbolQualities::is_const()
{
	return const_q;
}

bool SymbolQualities::is_final()
{
	return final_q;
}

bool SymbolQualities::is_dynamic()
{
	return dynamic_q;
}

bool SymbolQualities::is_static()
{
	return static_q;
}

bool SymbolQualities::is_signed()
{
	return signed_q;
}

bool SymbolQualities::is_unsigned()
{
	return unsigned_q;
}

void SymbolQualities::add_qualities(std::vector<SymbolQuality> to_add)
{
	// simply populate the vector; since we are adding, we don't really care about the original values
	for (std::vector<SymbolQuality>::iterator it = to_add.begin(); it != to_add.end(); it++)
	{
		this->add_quality(*it);
	}
}

void SymbolQualities::add_quality(SymbolQuality to_add)
{
    // Add a single quality to our qualities list
    if (to_add == CONSTANT) {
        const_q = true;

		// we cannot have final and const together
		if (final_q) throw CompilerException("Quality conflict");	// todo: proper exception type
    } else if (to_add == FINAL) {
		final_q = true;

		// we cannot have final and const together
		if (const_q) throw CompilerException("Quality conflict");	// todo: proper exception type
	} else if (to_add == STATIC) {
        static_q = true;
    } else if (to_add == DYNAMIC) {
        dynamic_q = true;
    } else if (to_add == SIGNED) {
        signed_q = true;
    } else if (to_add == UNSIGNED) {
        unsigned_q = true;
        signed_q = false;
	}
	else if (to_add == LONG) {
		long_q = true;
		short_q = false;
	}
	else if (to_add == SHORT) {
		long_q = false;
		short_q = true;
	}
	else {
		// invalid quality; throw an exception
		throw CompilerException("Quality conflict");	// todo: proper exception type
	}
}

SymbolQualities::SymbolQualities(std::vector<SymbolQuality> qualities)
{
	// start with our default values
	const_q = false;
	final_q = false;
	static_q = false;
	dynamic_q = false;
	signed_q = false;
	unsigned_q = false;

	// then, populate according to the vector
	for (std::vector<SymbolQuality>::iterator it = qualities.begin(); it != qualities.end(); it++)
	{
		if (*it == CONSTANT)
		{
			const_q = true;
		}
		else if (*it == FINAL)
		{
			final_q = true;
		}
		else if (*it == STATIC)
		{
			static_q = true;
		}
		else if (*it == DYNAMIC)
		{
			dynamic_q = true;
		}
		else if (*it == SIGNED)
		{
			signed_q = true;
		}
		else if (*it == UNSIGNED)
		{
			unsigned_q = true;
		}
		else {
			continue;
		}
	}
}

SymbolQualities::SymbolQualities(bool is_const, bool is_static, bool is_dynamic, bool is_signed, bool is_unsigned, bool is_long, bool is_short) :
	const_q(is_const),
	static_q(is_static),
	dynamic_q(is_dynamic),
	signed_q(is_signed),
	unsigned_q(is_unsigned),
	long_q(is_long),
	short_q(is_short)
{
	// unsigned always wins out over signed
	if (SymbolQualities::unsigned_q) {
		SymbolQualities::signed_q = false;
	}

	// const will always win out over static and dynamic
	if (SymbolQualities::const_q) {
		SymbolQualities::static_q, SymbolQualities::dynamic_q = false;
	}

	// if both long and short are set, generate a warning
	if (SymbolQualities::long_q && SymbolQualities::short_q) {
		// todo: warning
		std::cerr << "Warning: 'long' and 'short' both used as qualifiers; this amounts to a regular integer" << std::endl;

		// delete both qualities
		SymbolQualities::long_q = false;
		SymbolQualities::short_q = false;
	}
}

SymbolQualities::SymbolQualities()
{
	// everything defaults to false if nothing is given
	const_q = false;
	final_q = false;
	static_q = false;
	dynamic_q = false;
	signed_q = false;
	unsigned_q = false;
	long_q = false;
	short_q = false;
}

SymbolQualities::~SymbolQualities()
{

}

/*

DataType methods

*/

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
	return (this->primary == right[0]) && (this->subtype == right[1]);
}

bool DataType::operator!=(const Type right[2])
{
	return (this->primary != right[0]) || (this->subtype != right[1]);
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
	
	Compares 'self' with 'to_compare'. Types are compatible if one of the following is true:
		- if pointer or array type:
			- subtypes are equal
			- one of the subtypes is RAW
		- left OR right is RAW

	*/

	if (this->primary == RAW || to_compare.get_primary() == RAW) {
		return true;
	}
	else if ((this->primary == PTR && to_compare.get_primary() == PTR) || (this->primary == ARRAY && to_compare.get_primary() == ARRAY))
	{
		// cast the subtypes to DataType (with a subtype of NONE) and call is_compatible on them
		return static_cast<DataType>(this->subtype).is_compatible(static_cast<DataType>(to_compare.get_subtype()));
	}
	else {
		Type right;
		Type left;

		if (this->primary == ARRAY)
		{
			right = this->subtype;
		}
		else {
			right = this->primary;
		}

		if (to_compare.get_primary() == ARRAY)
		{
			left = to_compare.get_subtype();
		}
		else {
			left = to_compare.get_primary();
		}

		return right == left;
	}
}

Type DataType::get_primary() const
{
	return this->primary;
}

Type DataType::get_subtype() const
{
	return this->subtype;
}

SymbolQualities DataType::get_qualities() const {
	return this->qualities;
}

size_t DataType::get_array_length() const {
	return this->array_length;
}

std::string DataType::get_struct_name() const {
	return this->struct_name;
}

void DataType::set_primary(Type new_primary) {
	this->primary = new_primary;
}

void DataType::set_subtype(Type new_subtype) {
	this->subtype = new_subtype;
}

void DataType::add_qualities(std::vector<SymbolQuality> to_add) {
	// simply use the "SymbolQualities::add_qualities" function
	this->qualities.add_qualities(to_add);

    // update the width
    this->set_width();
}

void DataType::add_quality(SymbolQuality to_add) {
    // set the qualities of this variable to
    this->qualities.add_quality(to_add);

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

DataType::DataType(Type primary, Type subtype, std::vector<SymbolQuality> qualities, size_t array_length) :
	primary(primary),
	subtype(subtype),
	array_length(array_length)
{
	// add our symbol qualities
	this->qualities = SymbolQualities(qualities);
	
	// if the type is int, set signed to true if it is not unsigned
	if (primary == INT && !this->qualities.is_unsigned()) {
		this->add_qualities({ SIGNED });
	}
	else if (primary == FLOAT)
	{
		this->add_qualities({ SIGNED });
	}

	// set the data width
	this->set_width();
}

DataType::DataType(Type primary, Type subtype, SymbolQualities qualities, size_t array_length, std::string struct_name) :
    primary(primary),
    subtype(subtype),
    qualities(qualities),
	array_length(array_length),
	struct_name(struct_name)
{
    // if the type is int, set signed to true if it is not unsigned
	if (primary == INT && !this->qualities.is_unsigned()) {
		this->add_qualities({ SIGNED });
	}
	else if (primary == FLOAT)
	{
		this->add_qualities({ SIGNED });
	}

	// set the data width
	this->set_width();
}

DataType::DataType()
{
	this->primary = NONE;
	this->subtype = NONE;
	this->qualities = SymbolQualities();	// no qualities to start
	this->array_length = 0;
	this->struct_name = "";
}

DataType::~DataType()
{

}
