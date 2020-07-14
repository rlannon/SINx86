/*

SIN Toolchain (x86 target)
symbol_qualities.cpp
Copyright 2020 Riley Lannon

Implementation of the SymbolQualities class

*/

#include "symbol_qualities.h"

const std::unordered_map<std::string, SymbolQuality> symbol_qualities::quality_strings = {
	{ "const", CONSTANT },
	{ "final", FINAL },
	{ "static", STATIC },
	{ "dynamic", DYNAMIC },
	{ "long", LONG },
	{ "short", SHORT },
	{ "signed", SIGNED },
	{ "unsigned", UNSIGNED },
	{ "sincall", SINCALL_CONVENTION },
	{ "c64", C64_CONVENTION },
	{ "windows", WINDOWS_CONVENTION },
	{ "extern", EXTERN }
};

bool symbol_qualities::operator==(const symbol_qualities& right) const {
	return (
		(this->long_q == right.long_q) &&
		(this->short_q == right.short_q) &&
		(this->signed_q == right.signed_q) &&
		(this->unsigned_q == right.unsigned_q) &&
		(this->const_q == right.const_q) &&
		(this->final_q == right.final_q) &&
		(this->dynamic_q == right.dynamic_q) &&
		(this->extern_q == right.extern_q) &&
		(this->c64_con == right.c64_con) &&
		(this->windows_con == right.windows_con) &&
		(this->sincall_con == right.sincall_con)
	);
}

bool symbol_qualities::operator!=(const symbol_qualities& right) const {
	return !this->operator==(right);
}

bool symbol_qualities::is_long()
{
    return long_q;
}

bool symbol_qualities::is_short()
{
    return short_q;
}

bool symbol_qualities::is_const()
{
	return const_q;
}

bool symbol_qualities::is_final()
{
	return final_q;
}

bool symbol_qualities::is_dynamic()
{
	return dynamic_q;
}

bool symbol_qualities::is_static()
{
	return static_q;
}

bool symbol_qualities::is_signed()
{
	return signed_q;
}

bool symbol_qualities::is_unsigned()
{
	return unsigned_q;
}

bool symbol_qualities::is_extern()
{
	return extern_q;
}

bool symbol_qualities::is_sincall()
{
	return sincall_con;
}

bool symbol_qualities::is_c64()
{
	return c64_con;
}

bool symbol_qualities::is_windows()
{
	return windows_con;
}

void symbol_qualities::add_qualities(symbol_qualities to_add) {
	// combines two SymbolQualities objects

	// todo: refactor how qualities are stored in SymbolQualities so that we can simplify this

	if (to_add.is_const()) this->add_quality(CONSTANT);
	if (to_add.is_final()) this->add_quality(FINAL);
	if (to_add.is_static()) this->add_quality(STATIC);
	if (to_add.is_dynamic()) this->add_quality(DYNAMIC);
	if (to_add.is_long()) this->add_quality(LONG);
	if (to_add.is_short()) this->add_quality(SHORT);
	if (to_add.is_signed()) this->add_quality(SIGNED);
	if (to_add.is_unsigned()) this->add_quality(UNSIGNED);
	if (to_add.is_sincall()) this->add_quality(SINCALL_CONVENTION);
	if (to_add.is_c64()) this->add_quality(C64_CONVENTION);
	if (to_add.is_windows()) this->add_quality(WINDOWS_CONVENTION);
	if (to_add.is_extern()) this->add_quality(EXTERN);
}

void symbol_qualities::add_quality(SymbolQuality to_add)
{
    // Add a single quality to our qualities list
    if (to_add == CONSTANT) {
        const_q = true;

		// we cannot have final and const together
		if (final_q) throw std::string("const");	// todo: proper exception type
    } else if (to_add == FINAL) {
		final_q = true;

		// we cannot have final and const together
		if (const_q) throw std::string("final");	// todo: proper exception type
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
	else if (to_add == SINCALL_CONVENTION) {
		sincall_con = true;
		c64_con = false;
		windows_con = false;
	}
	else if (to_add == C64_CONVENTION) {
		sincall_con = false;
		c64_con = true;
		windows_con = false;
	}
	else if (to_add == WINDOWS_CONVENTION) {
		sincall_con = false;
		windows_con = true;
	}
	else if (to_add == EXTERN) {
		extern_q = true;
	}
	else {
		// invalid quality; throw an exception
		throw CompilerException("Quality conflict");	// todo: proper exception type
	}
}

symbol_qualities::symbol_qualities(std::vector<SymbolQuality> qualities)
{
	// start with our default values
	this->const_q = false;
	this->final_q = false;
	this->static_q = false;
	this->dynamic_q = false;
	this->signed_q = false;
	this->unsigned_q = false;
	this->sincall_con = false;
	this->c64_con = false;
	this->windows_con = false;
	this->extern_q = false;

	// todo: there must be a better way of doing this

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
		else if (*it == SINCALL_CONVENTION)
		{
			sincall_con = true;
		}
		else if (*it == C64_CONVENTION)
		{
			c64_con = true;
		}
		else if (*it == WINDOWS_CONVENTION) {
			windows_con = true;
		}
		else if (*it == EXTERN) {
			extern_q = true;
		}
		else {
			continue;
		}
	}
}

symbol_qualities::symbol_qualities(bool is_const, bool is_static, bool is_dynamic, bool is_signed, bool is_unsigned, bool is_long, bool is_short, bool is_extern) :
	const_q(is_const),
	static_q(is_static),
	dynamic_q(is_dynamic),
	signed_q(is_signed),
	unsigned_q(is_unsigned),
	long_q(is_long),
	short_q(is_short),
	extern_q(is_extern)
{
	// unsigned always wins out over signed
	if (this->unsigned_q) {
		this->signed_q = false;
	}

	// const will always win out over dynamic (can be 'static const')
	if (this->const_q) {
		this->dynamic_q = false;
	}
	this->final_q = false;

	// if both long and short are set, generate a warning
	if (this->long_q && this->short_q) {
		// todo: warning
		std::cerr << "Warning: 'long' and 'short' both used as qualifiers; this amounts to a regular integer" << std::endl;

		// delete both qualities
		this->long_q = false;
		this->short_q = false;
	}

	this->sincall_con = false;
	this->c64_con = false;
	this->windows_con = false;
}

symbol_qualities::symbol_qualities()
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
	sincall_con = false;
	c64_con = false;
	windows_con = false;
	extern_q = false;
}

symbol_qualities::~symbol_qualities()
{

}
