/*

SIN Toolchain
DataType.h
Copyright 2020 Riley Lannon

Contains the definitions of the DataType class

DataType contains the type, subtype, and qualities of a given expression alongside methods to evaluate and compare it.

*/

#pragma once

#include <vector>
#include <cinttypes>
#include <cstdlib>
#include <memory>

#include "data_widths.h"
#include "EnumeratedTypes.h"
#include "Exceptions.h"
#include "symbol_qualities.h"

class Expression;	// since 'DataType' uses 'shared_ptr<Expression>' to track array length, we must forward-declare 'Expression' here

class DataType
{
	Type primary;	// always has a primary type
	std::shared_ptr<DataType> subtype;	// may or may not have a subtype; nullptr if no subtype is present

	symbol_qualities qualities;	// the qualities of the symbol (const, signed, etc.)
	size_t array_length;	// if it's an array, track the length
	size_t width;	// the width (in bytes) of the type

	std::shared_ptr<Expression> array_length_expression;

	std::string struct_name;	// if the data type is 'struct', we need to know its name so we can look it up in the struct table

	void set_width();	// sets the symbol's type based on the primary type
public:
	DataType& operator=(const DataType &right);

	bool operator==(const DataType right);
	bool operator!=(const DataType right);

	bool operator==(const Type right[2]);
	bool operator!=(const Type right[2]);

	bool operator==(const Type right);
	bool operator!=(const Type right);

	Type get_primary() const;
	Type get_subtype() const;
	symbol_qualities get_qualities() const;
	size_t get_array_length() const;
	std::string get_struct_name() const;

	std::shared_ptr<Expression> get_array_length_expression() const;

	std::shared_ptr<DataType> get_full_subtype() const;

	void set_primary(Type new_primary);
	void set_subtype(DataType new_subtype);
	void set_subtype(std::shared_ptr<DataType> new_subtype);

	void set_array_length(size_t new_length);

	void add_qualities(symbol_qualities to_add);
    void add_quality(SymbolQuality to_add);

	void set_struct_name(std::string name);

	bool is_compatible(DataType to_compare) const;

	size_t get_width() const;

	static bool is_valid_type(DataType &t);

    DataType(Type primary, DataType subtype, symbol_qualities qualities, std::shared_ptr<Expression> array_length_exp = nullptr, std::string struct_name = "");
	DataType(Type primary);
	DataType(const DataType &ref);
	DataType();
	~DataType();
};
