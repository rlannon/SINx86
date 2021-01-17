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
#include "util/Exceptions.h"

static const std::string VERSION = "0.0.0a";
static const std::string YEAR = "2021";

int main (int argc, char **argv) {
	// Create the argument parser
	args::ArgumentParser parser("Compiler for the SIN programming language.", "See the GitHub repository for bug tracking, documentation, etc.");
	args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
	args::CompletionFlag completion(parser, {"complete"});
	args::Flag version(parser, "version", "Get the program's version number", {"version"}, args::Options::KickOut);

	// File name options
	args::Positional<std::string> filename(parser, "filename", "The .sin file to compile");
	args::ValueFlag<std::string> outfile(parser, "outfile", "Specify an output assmembly file", {'o', "outfile"});

	// Compiler mode options
	args::Flag use_micro(parser, "micro", "Compile in uSIN mode", {"micro"});
	args::ValueFlag<std::string> mode(parser, "mode", "Determines how strict the compiler is; accepted options are 'lax', 'normal', or 'strict'", {'m', "mode"});

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
		// see if we asked for the version; if so, print it and exit (ignores all other flags)
		if (version)
		{
			std::cout << "SIN (sinx86) " << VERSION << "\nCopyright (C) " << YEAR << " Riley Lannon" << std::endl;
			return 0;
		}
		
		// check to see if an input file was specified; if not, exit
		std::string infile_name;
		if (filename)
		{
			infile_name = args::get(filename);
		}
		else
		{
			std::cout << "No input file specified." << std::endl;
			return 1;
		}
		

		// get the compiler mode
		std::string compiler_mode{ mode ? args::get(mode) : "normal" };
		bool allow_unsafe;
		bool use_strict = (compiler_mode == "strict");
		if (
			compiler_mode == "normal" ||
			use_strict
		) {
			allow_unsafe = false;
		}
		else if (compiler_mode == "lax")
		{
			allow_unsafe = true;
		}
		else
		{
			throw CompilerException("Argument error: unknown compiler mode '" + compiler_mode + "'");
		}
		
		bool compile_micro = (use_micro ? args::get(use_micro) : false);

		// get the name for the generated assembly file
        // remove the extension from the file name and append ".s"
		std::string outfile_name;

		if (outfile)
		{
			outfile_name = args::get(outfile);
		}
		else
		{
			size_t last_index = infile_name.find_last_of(".");
			if (last_index != std::string::npos)
				outfile_name = infile_name.substr(0, last_index);
			outfile_name += ".s";
		}

		// create our compiler
		compiler c { allow_unsafe, use_strict, compile_micro };
		c.generate_asm(infile_name, outfile_name);
	}
	catch (std::exception &e) {
        std::cout << "Exception occurred: " << e.what() << std::endl;
        return 1;
    }

	return 0;
}
