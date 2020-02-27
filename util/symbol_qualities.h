/*

SIN Toolchain (x86 target)
symbol_qualities.h
Copyright 2020 Riley Lannon

The definition of the classes pertaining to symbol qualities

*/

#pragma once

#include <unordered_map>
#include <vector>
#include <string>

#include "EnumeratedTypes.h"
#include "Exceptions.h"

class symbol_qualities
{
	bool const_q;	// our qualities -- since almost all of these are reserved in C++, suffix with _q, for "quality"
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
	void add_qualities(symbol_qualities to_add);
    void add_quality(SymbolQuality to_add);

	symbol_qualities(std::vector<SymbolQuality> qualities);
	symbol_qualities(bool is_const, bool is_static, bool is_dynamic, bool is_signed, bool is_unsigned, bool is_long = false, bool is_short = false);
	symbol_qualities();
	~symbol_qualities();
};
