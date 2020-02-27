/*

SIN Toolchain
main.cpp
Copyright 2020 Riley Lannon

For a documentation of the language, see either the doc folder in this project or the Github wiki pages.

*/

// All pre-existing headers used in this file are already included in other project files

#include <fstream>
#include <iostream>

// Our headers
#include "parser/Parser.h"
#include "compile/compiler.h"


void file_error(std::string filename) {
	std::cerr << "**** Cannot open file '" + filename + "'!" << "\n\n" << "Press enter to exit..." << std::endl;
	std::cin.get();
	return;
}

int main (int argc, char *argv[]) {
    // current usage: sinx86

    // create new compiler and parser objects

    try {
        // todo: refactor constructors to streamline this process
        std::ifstream infile;
        infile.open("samples/sample.sin", std::ios::in);
        Lexer l(infile);
        Parser *p = new Parser(l);
        compiler *c = new compiler();
        
        // compile our code
        c->generate_asm("samples/sample.sin", *p);

        // clean-up
        infile.close();
        delete p;
        delete c;

        return 0;
    } catch (std::exception &e) {
        std::cout << "Exception occurred: " << e.what() << std::endl;
        exit(1);
    }
}
