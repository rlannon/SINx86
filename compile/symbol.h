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
#include "compile_util/register_usage.h"

class symbol {
    /*

    The base class for our symbols    

    */
    
    unsigned int stack_offset;  // the offset, in bytes, from the stack frame base
    reg current_reg;    // current register holding the symbol
protected:
    SymbolType symbol_type;

    std::string name;
    std::string scope_name; // the name of the scope -- can be "global" or a function name
    unsigned int scope_level;   // the _level_ of the scope -- allows for block scopes

    DataType type;  // the symbol's type

    bool initialized;   // whether the data was initialized
    bool freed; // whether the memory has been allocated or freed
public:
    // our getters
    SymbolType get_symbol_type() const;

    reg get_register() const;
    void set_register(reg to_set);

    std::string get_name() const;
    std::string get_scope_name() const;
    unsigned int get_scope_level() const;

    DataType get_data_type() const;
    unsigned int get_stack_offset() const;

    // note these won't have any effect on functions (as functions are not allocated in SIN)
    bool was_initialized() const;
    bool was_freed() const;

    void set_initialized();
    void free();

    // constructors
    explicit symbol(std::string name, std::string scope_name, unsigned int scope_level, DataType type_information, unsigned int stack_offset);
    symbol();
    virtual ~symbol(); // the destructor must be virtual for the sake of the child class
};
