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

symbol& struct_info::get_member(std::string name)
{
	/*
	
	get_member
	Gets the symbol information for the member of a given struct
	
	*/

	try {
		std::shared_ptr<symbol> sym = this->members.find(name);
		return *sym;
	}
	catch (std::exception& e) {
		throw SymbolNotFoundException(0);	// todo: line number?
	}
}

size_t struct_info::get_width() const {
	return this->struct_width;
}

struct_info::struct_info(std::string name, std::vector<symbol> members, unsigned int line) {
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

    for (symbol s: members) {
        try {
            this->members.insert(std::make_shared<symbol>(s));

            size_t sym_width = s.get_data_type().get_width();
            if (sym_width == 0) {
				if (s.get_data_type().get_qualities().is_dynamic()) {
					sym_width = sin_widths::PTR_WIDTH;
				}
                else if (s.get_data_type().get_primary() == ARRAY) {
                    sym_width = s.get_data_type().get_array_length();
                }
				else {
					this->width_known = false;	// todo: should this throw an error?
				}
            }

            this->struct_width += sym_width;
        } catch (std::exception &e) {
            throw DuplicateSymbolException(line);
        }
    }
}

struct_info::struct_info(std::string struct_name) {
    this->struct_name = struct_name;
    this->struct_width = 0;
    this->width_known = false;
}

struct_info::struct_info() {
    this->struct_name = "";
    this->struct_width = 0;
    this->width_known = false;
}

struct_info::~struct_info() {
    // todo: destructor
}
