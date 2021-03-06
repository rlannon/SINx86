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
	std::vector<DataType> contained_types;	// tuples can have multiple contained types; will be empty if no subtype exists

	symbol_qualities qualities;	// the qualities of the symbol (const, signed, etc.)
	size_t array_length;	// if it's an array, track the length
	size_t width;	// the width (in bytes) of the type

	std::shared_ptr<Expression> array_length_expression;

	std::string struct_name;	// if the data type is 'struct', we need to know its name so we can look it up in the struct table

	void set_width();	// sets the symbol's type based on the primary type
    void set_must_free();   // sets _must_free based on data about the symbol

    bool _must_free;
public:
	static const bool is_valid_type_promotion(const symbol_qualities& left, const symbol_qualities& right);

	DataType& operator=(const DataType &right);

	bool operator==(const DataType& right) const;
	bool operator!=(const DataType& right) const;

	bool operator==(const Type right) const;
	bool operator!=(const Type right) const;

	Type get_primary() const;
	DataType get_subtype() const;
	const std::vector<DataType> &get_contained_types() const;
	std::vector<DataType> &get_contained_types();
	bool has_subtype() const;
	
	const symbol_qualities& get_qualities() const;
	symbol_qualities& get_qualities();

	size_t get_array_length() const;
	std::string get_struct_name() const;

	const Expression *get_array_length_expression() const;

	void set_primary(Type new_primary);
	void set_subtype(DataType new_subtype);
	void set_contained_types(std::vector<DataType> types_list);

	void set_array_length(size_t new_length);

	void add_qualities(symbol_qualities to_add);
    void add_quality(SymbolQuality to_add);

	void set_struct_name(std::string name);

	bool is_compatible(DataType to_compare) const;

	size_t get_width() const;

	static bool is_valid_type(const DataType &t);
	bool is_reference_type() const;

	virtual bool must_initialize() const;
    bool must_free() const;

    DataType(Type primary, DataType subtype, symbol_qualities qualities, std::shared_ptr<Expression> array_length_exp = nullptr, std::string struct_name = "");
	DataType (Type primary, std::vector<DataType> contained_types, symbol_qualities qualities);
	DataType(Type primary);
	DataType(const DataType &ref);
	DataType();
	~DataType();
};
