/*

SIN Toolchain (x86 target)
struct_info.cpp
Copyright 2020 Riley Lannon

The implementation of the struct_info class

*/

#include "struct_info.h"

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
            this->members.insert(std::make_pair(s.get_name(), s));

            size_t sym_width = s.get_data_type().get_width();
            if (sym_width != 0) {
                this->struct_width += s.get_data_type().get_width();
            } else {
                this->width_known = false;
            }
        } catch (std::exception &e) {
            throw DuplicateSymbolException(line);
        }
    }
}

struct_info::struct_info() {
    this->struct_name = "";
    this->struct_width = 0;
    this->width_known = false;
}

struct_info::~struct_info() {
    // todo: destructor
}
