/*

SIN Toolchain
main.cpp
Copyright 2020 Riley Lannon

For a documentation of the language, see either the doc folder in this project or the Github wiki pages.

*/

// C++/STL headers
#include <fstream>
#include <iostream>

// Custom headers
#include "parser/Parser.h"
#include "compile/compiler.h"
#include "compile/compile_util/constant_eval.h"


void file_error(std::string filename) {
	std::cerr << "**** Cannot open file '" + filename + "'!" << "\n\n" << "Press enter to exit..." << std::endl;
	std::cin.get();
	return;
}

int main (int argc, char *argv[]) {
	// give a default filename
	std::string sin_file = "samples/sample.sin";

	// if a filename was supplied, it should be the first argument
	if (argc > 1) {
		sin_file = std::string(argv[1]);
	}

	try {
		// todo: refactor constructors to streamline this process
		std::ifstream infile;
		infile.open(sin_file, std::ios::in);
		Lexer l(infile);
		Parser *p = new Parser(l);
		compiler *c = new compiler();

		// compile our code
		c->generate_asm(sin_file, *p);

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
