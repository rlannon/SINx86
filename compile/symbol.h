/*

SIN Toolchain (x86 target)
symbol.h

The class for compiler symbols

*/

#pragma once

#include <vector>
#include <utility>
#include <string>

#include "../util/DataType.h"   // For all information about types

class symbol {
    /*

    The base class for our symbols    

    */
    
    unsigned int stack_offset;  // the offset, in bytes, from the stack frame base
protected:
    SymbolType symbol_type;

    std::string name;
    std::string scope_name; // the name of the scope -- can be "global" or a function name
    unsigned int scope_level;   // the _level_ of the scope -- allows for block scopes

    DataType type;  // the symbol's type
public:
    // our getters
    SymbolType get_symbol_type() const;

    std::string get_name() const;
    std::string get_scope_name() const;
    unsigned int get_scope_level() const;

    DataType get_data_type() const;
    unsigned int get_stack_offset() const;

    // constructors
    explicit symbol(std::string name, std::string scope_name, unsigned int scope_level, DataType type_information, unsigned int stack_offset);
    symbol();
    virtual ~symbol(); // the destructor must be virtual for the sake of the child class
};

class function_symbol: public symbol {
    /*

    The class for function symbols
    todo: write more info on the 

    */

    // Function arguments -- formal parameters should be stored as symbols (they are considered local variables, so they will be pushed first)
    std::vector<symbol> formal_parameters;
public:
    function_symbol(std::string function_name, DataType return_type, std::vector<symbol> formal_parameters);

    // defaults
    function_symbol();
    ~function_symbol();
};
