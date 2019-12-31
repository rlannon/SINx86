/*

SIN Toolchain (x86 target)
compile.cpp

Implementation of the compiler class

Copyright 2019 Riley Lannon

*/

#include "compile.h"

void compiler::generate_asm(std::string filename, Parser &p) {
    /*

    generate_asm
    The compiler's entry function

    Generates x86 code for the program, in NASM syntax, and writes it to a file.
    The outfile will have the same name as the .sin file being compiled, the exception being a .s extension instead of .sin
    Note the parser supplied will generate the AST here

    @param  filename    The name of the file we wish to compile
    @param  p   The parser object that will be used to parse the file
    @return None

    */

    // catch parser exceptions here
    try {
        // create our abstract syntax tree
        StatementBlock ast = p.create_ast();

        // todo: compile the AST to x86
    } catch (std::exception &e) {
        // todo: exception handling should be improved
        std::cout << "An error occurred while parsing the file:" << std::endl;
        std::cout << e.what() << std::endl;
    }
}

bool compiler::is_in_scope(symbol &sym) {
    /*

    Determines whether the symbol in question is available in the current scope

    A valid symbol will be:
        - In the "global" scope OR in the current scope
        - Have a scope level less than or equal to the current level
    or:
        - Be a static variable

    */

    return ( sym.get_data_type().get_qualities().is_static() ||
        (( sym.get_scope_name() == "global" || 
            sym.get_scope_name() == this->current_scope_name
        ) && sym.get_scope_level() <= this->current_scope_level)
    );
}

compiler::compiler() {
    // todo: constructor
}

compiler::~compiler() {
    // todo: destructor
}
