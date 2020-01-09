/*

SIN Toolchain
DataType.h
Copyright 2019 Riley Lannon

Contains the definitions of the 'SymbolQualities' and 'DataType' classes.
SymbolQualities contains the object used by the DataType class to store qualities so std::vector is not needed
DataType contains the type, subtype, and qualities of a given expression alongside methods to evaluate and compare it.

*/

#pragma once

#include <vector>
#include <cinttypes>
#include <cstdlib>

#include "data_widths.h"
#include "EnumeratedTypes.h"
#include "Exceptions.h"

class SymbolQualities
{
	bool const_q;	// our qualities -- since these are almost all reserved in C++, suffix with _q, for "quality"
	bool final_q;
	bool static_q;
	bool dynamic_q;
	bool signed_q;
	bool unsigned_q;
	bool long_q;
	bool short_q;
public:
	static const std::unordered_map<std::string, SymbolQuality> quality_strings;

	bool is_const();	// accessors
	bool is_final();
	bool is_static();
	bool is_dynamic();
	bool is_signed();
	bool is_unsigned();
	bool is_long();
	bool is_short();

	// void add_qualities(std::vector<SymbolQuality> to_add);
	void add_qualities(SymbolQualities to_add);
    void add_quality(SymbolQuality to_add);

	SymbolQualities(std::vector<SymbolQuality> qualities);
	SymbolQualities(bool is_const, bool is_static, bool is_dynamic, bool is_signed, bool is_unsigned, bool is_long = false, bool is_short = false);
	SymbolQualities();
	~SymbolQualities();
};

class DataType
{
	Type primary;	// always has a primary type
	Type subtype;	// may or may not have a subtype
	SymbolQualities qualities;	// the qualities of the symbol (const, signed, etc.)
	size_t array_length;	// if it's an array, track the length
	size_t width;	// the width (in bytes) of the type

	std::string struct_name;	// if the data type is 'struct', we need to know its name so we can look it up in the struct table

	void set_width();	// sets the symbol's type based on the primary type
public:
	bool operator==(const DataType right);
	bool operator!=(const DataType right);

	bool operator==(const Type right[2]);
	bool operator!=(const Type right[2]);

	bool operator==(const Type right);
	bool operator!=(const Type right);

	Type get_primary() const;
	Type get_subtype() const;
	SymbolQualities get_qualities() const;
	size_t get_array_length() const;
	std::string get_struct_name() const;

	void set_primary(Type new_primary);
	void set_subtype(Type new_subtype);
	void add_qualities(SymbolQualities to_add);
    void add_quality(SymbolQuality to_add);
	void set_struct_name(std::string name);

	bool is_compatible(DataType to_compare) const;

	size_t get_width() const;

    DataType(Type primary, Type subtype, SymbolQualities qualities, size_t array_length = 0, std::string struct_name = "");
	DataType(Type primary);
	DataType();
	~DataType();
};
