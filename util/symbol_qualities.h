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
    bool _listed_unsigned;
	bool long_q;
	bool short_q;
	bool extern_q;
    bool _managed;

	// function qualities -- for calling conventions, unused by other data
	// todo: create additional, inherited class 'function_symbol_qualities' to use with functions?
	bool sincall_con;
	bool c64_con;
	bool windows_con;
public:
	bool operator==(const symbol_qualities& right) const;
	bool operator!=(const symbol_qualities& right) const;

	static const std::unordered_map<std::string, SymbolQuality> quality_strings;

	bool is_const();	// accessors
	bool is_final();
	bool is_static();
	bool is_dynamic();
	bool is_signed();
	bool is_unsigned();
	bool is_long();
	bool is_short();
	bool is_extern();
    bool is_managed();

	// function-specific qualities
	bool is_sincall();
	bool is_c64();
	bool is_windows();

    bool has_sign_quality();

	// void add_qualities(std::vector<SymbolQuality> to_add);
	void add_qualities(symbol_qualities to_add);
    void add_quality(SymbolQuality to_add);

	symbol_qualities(std::vector<SymbolQuality> qualities);
	symbol_qualities(bool is_const, bool is_static, bool is_dynamic, bool is_signed, bool is_long = false, bool is_short = false, bool is_extern = false);
	symbol_qualities();
	~symbol_qualities();
};
