/*

SIN Toolchain
main.cpp
Copyright 2020 Riley Lannon

For a documentation of the language, see either the doc folder in this project or the Github wiki pages.

*/

// C++/STL headers
#include <fstream>
#include <iostream>

// Third-party libraries
#include <args.hxx>

// Custom headers
#include "parser/Parser.h"
#include "compile/compiler.h"
#include "compile/compile_util/constant_eval.h"


int main (int argc, char **argv) {
	// Create the argument parser
	args::ArgumentParser parser("Compiler for the SIN programming language.", "See the GitHub repository for bug tracking, documentation, etc.");
	args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
	args::CompletionFlag completion(parser, {"complete"});

	args::Positional<std::string> filename(parser, "filename", "The .sin file to compile", args::Options::Required);
	args::Flag use_micro(parser, "micro", "Compile in uSIN mode", {"micro"});
	args::ValueFlag<std::string> mode(parser, "mode", "Determines how strict the compiler is", {'m', "mode"});

	// parse arguments
	try {
		parser.ParseCLI(argc, argv);
	}
	catch (args::Help)
	{
		std::cout << parser;
		return 0;
	}
	catch (args::ParseError &e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}
	catch (args::ValidationError &e)
	{
		std::cerr << e.what() << std::endl;
		std::cerr << parser;
		return 1;
	}

	// run the program
	try {
		std::string compiler_mode{ mode ? args::get(mode) : "normal" };
		compiler c;
		c.generate_asm(args::get(filename));
	}
	catch (std::exception &e) {
        std::cout << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }

	return 0;
}
