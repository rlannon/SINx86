/*

SIN Toolchain (x86 target)
struct_info.cpp
Copyright 2020 Riley Lannon

The implementation of the struct_info class

*/

#include "struct_info.h"

bool struct_info::is_width_known() const {
	return this->width_known;
}

std::string struct_info::get_struct_name() const
{
	return this->struct_name;
}

symbol *struct_info::get_member(std::string name)
{
	/*
	
	get_member
	Gets the symbol information for the member of a given struct
	
	*/

    symbol *s = &this->members.find(name, this->struct_name);
    return s;
}

size_t struct_info::get_width() const {
	return this->struct_width;
}

struct_info::struct_info(std::string name, std::vector<std::shared_ptr<symbol>> members, unsigned int line) {
    /*
    
        struct_info
        Specialized constructor

        Populates the hash table with the struct members and calculates the struct's width
        
        @param  name    The name of the struct
        @param  members The data members, all symbols created through allocation statements
        @param  line    The line where the struct definition can be found
        @return None

    */

    this->struct_name = name;
    this->struct_width = 0;
    this->width_known = true;

    for (auto s: members) {
        try {
            this->members.insert(s);

            size_t sym_width = s->get_data_type().get_width();
            if (sym_width == 0) {
				if (s->get_data_type().get_qualities().is_dynamic()) {
					sym_width = sin_widths::PTR_WIDTH;
				}
                else if (s->get_data_type().get_primary() == ARRAY) {
                    sym_width = s->get_data_type().get_array_length();
                }
				else {
					this->width_known = false;	// todo: should this throw an error?
				}
            }

            // skip adding the width if we have a function -- they aren't allocated with the struct
            if (s->get_symbol_type() != FUNCTION_SYMBOL) {
                this->struct_width += sym_width;
            }
        } catch (DuplicateSymbolException &e) {
            e.set_line(line);
            throw e;
        }
    }
}

std::vector<std::shared_ptr<symbol>> struct_info::get_all_members() {
    // gets all struct members in a vector
    return this->members.get_all_symbols();
}

std::vector<symbol> struct_info::get_members_to_free(std::string scope_name, unsigned int scope_level) {
    // returns members that must be freed
    return this->members.get_symbols_to_free(scope_name, scope_level, false);
}

std::vector<symbol> &struct_info::get_members_to_free(std::vector<symbol> &current, std::string scope_name, unsigned int scope_level) {
    // calls overloaded version on 'members'
    return this->members.get_symbols_to_free(current, scope_name, scope_level, false);
}

struct_info::struct_info(std::string struct_name) {
    this->struct_name = struct_name;
    this->struct_width = 0;
    this->width_known = false;
}

struct_info::struct_info(const struct_info &s) {
    this->struct_name = s.struct_name;
    this->struct_width = s.struct_width;
    this->members = s.members;
    this->width_known = s.width_known;
}

struct_info::struct_info() {
    this->struct_name = "";
    this->struct_width = 0;
    this->width_known = false;
}

struct_info::~struct_info() {
    // todo: destructor
}
