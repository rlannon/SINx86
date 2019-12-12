/*

SIN Toolchain
main.cpp
Copyright 2019 Riley Lannon

This is the main file for the SIN Compiler application. SIN is a small programming language that I will be using
as a way to get practice in compiler writing.

For a documentation of the language, see either the doc folder in this project or the Github wiki pages.

*/

// All pre-existing headers used in this file are already included in other project files

// Our headers
#include "parser/Parser.h"


void file_error(std::string filename) {
	std::cerr << "**** Cannot open file '" + filename + "'!" << "\n\n" << "Press enter to exit..." << std::endl;
	std::cin.get();
	return;
}

int main (int argc, char* argv[]) {
	return 0;	
}