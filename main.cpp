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
	try {
		// todo: refactor constructors to streamline this process
		/*std::ifstream infile;
		infile.open("samples/sample.sin", std::ios::in);
		Lexer l(infile);
		Parser *p = new Parser(l);
		compiler *c = new compiler();

		// compile our code
		c->generate_asm("samples/sample.sin", *p);

		// clean-up
		infile.close();
		delete p;
		delete c;*/

		compile_time_evaluator *cte = new compile_time_evaluator();
		std::shared_ptr<Expression> e = std::make_shared<Literal>(BOOL, "true");

		DataType a_info(
			BOOL,
			DataType(),
			symbol_qualities(
				true,
				false,
				false,
				false,
				false,
				false,
				false
			)
		);
		Allocation a(
			a_info,
			"x",
			true,
			e
		);
		symbol s("x", "global", 0, a_info, 0);
		s.set_initialized();

		cte->add_constant(a, s);

		std::shared_ptr<Expression> e2 = std::make_shared<Unary>(
			std::make_shared<Unary>(
				std::make_shared<LValue>(
					"x"
					),
				exp_operator::NOT
				), exp_operator::NOT
		);

		try {
			std::cout << "test: " << -(-5) << std::endl;
			std::string x = cte->evaluate_expression(e2, "global", 0, 1);
			std::cout << "Evaluated: " << x << std::endl;
		}
		catch (std::exception & e) {
			std::cout << e.what() << std::endl;
		}
		delete cte;
		
        return 0;
    } catch (std::exception &e) {
        std::cout << "Exception occurred: " << e.what() << std::endl;
        exit(1);
    }
}
