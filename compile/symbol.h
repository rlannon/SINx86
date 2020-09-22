/*

SIN Toolchain (x86 target)
symbol.h

The class for compiler symbols

*/

#pragma once

#include <vector>
#include <utility>
#include <string>
#include <unordered_map>

#include "../util/DataType.h"   // For all information about types

class symbol {
    /*

    The base class for our symbols

    */
    
	// these are not necessary for child classes and so should remain private to 'symbol'
    int offset;  // the offset, in bytes, from the stack frame base or from the struct base, depending on what the symbol is used for
    reg current_reg;    // current register holding the symbol
    bool is_parameter;  // whether the symbol is a function parameter
protected:
    SymbolType symbol_type;

    std::string name;
    std::string scope_name; // the name of the scope -- can be "global" or a function name
    unsigned int scope_level;   // the _level_ of the scope -- allows for block scopes

    DataType type;  // the symbol's type

    bool defined;   // whether the symbol was defined

    bool initialized;   // whether the data was initialized
    bool freed; // whether the memory has been allocated or freed

    unsigned int line_defined; // the line on which this symbol was introduced
public:
    // to check whether two symbols are the same
    bool operator==(const symbol& right) const;
    bool operator!=(const symbol& right) const;

    // to update our offset
    void set_offset(int new_offset);    // must be public -- cannot be protected because it will be accessed through a pointer
    
    // our getters
    SymbolType get_symbol_type() const;

    reg get_register() const;
    void set_register(reg to_set);

    void set_as_parameter();

    std::string get_name() const;
    std::string get_scope_name() const;
    unsigned int get_scope_level() const;

    DataType get_data_type() const;
    int get_offset() const;

    bool is_defined() const;
    void set_defined();

    // note these won't have any effect on functions (as functions are not allocated in SIN)
    bool was_initialized() const;
    bool was_freed() const;

    void set_initialized();
    void free();

    void set_line(unsigned int l);
    unsigned int get_line_defined() const;

    // constructors
    explicit symbol(
        std::string name,
        std::string scope_name,
        unsigned int scope_level,
        DataType type_information,
        unsigned int offset,
        bool defined=true,
        unsigned int line_defined=0
    );
    //symbol(symbol &&r);
    symbol();
    virtual ~symbol(); // the destructor must be virtual for the sake of the child class
};
