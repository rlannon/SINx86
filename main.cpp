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
#include "vm/SINVM.h"
#include "compile/Compiler.h"
#include "link/Linker.h"
#include "util/SinObjectFile.h"



void file_error(std::string filename) {
	std::cerr << "**** Cannot open file '" + filename + "'!" << "\n\n" << "Press enter to exit..." << std::endl;
	std::cin.get();
	return;
}


void help(std::string flag="") {

}


// TODO: modularize int main()
// todo: redo command-line argument parsing -- perhaps using an external library
int main (int argc, char* argv[]) {
	// first, make a vector of strings to hold **argv 
	std::vector<std::string> program_arguments;

	// create a vector of SinObjectFile to hold any filenames that are passed as parameters -- allocate on the heap; we need it for the duration of the program
	std::vector<SinObjectFile>* objects_vector = new std::vector<SinObjectFile>;

	// if we didn't pass in any command-line parameters, we need to make sure we get the necessary information from the user
	if (argc == 1) {
		std::string parameters;
		std::cout << "Parameters: ";
		std::getline(std::cin, parameters);	// get the parameters as a string

		// now, turn that into a vector of strings using a delimiter (in this case, a space)
		// split the line into parts, if we can
		std::vector<std::string> string_delimited;	// to hold the parts of our string
		size_t position = 0;
		while ((position = parameters.find(' ')) != std::string::npos) {
			string_delimited.push_back(parameters.substr(0, parameters.find(' ')));	// push the string onto the vector
			parameters.erase(0, position + 1);	// erase the string from the beginning up to the next token
		}
		string_delimited.push_back(parameters);	// add the last thing (we iterated one time too few so we didn't try to erase more than what existed)

		// and set program_arguments equal to that new vector
		program_arguments = string_delimited;
	}
	// otherwise, just push all of **argv onto program_arguments
	else {
		for (size_t i = 1; i < argc; i++) {
			std::string current_arg(argv[i]);
			program_arguments.push_back(current_arg);
		}
	}

	// now, create booleans for our various flags and set them if necessary
	bool interpret = false;
	bool compile = false;
	bool compile_only = false;
	bool disassemble = false;
	bool assemble = false;	// will automatically assemble files with the -c flag; if --no-assemble is set, it will clear it
	bool link = false;
	bool execute = false;
	bool debug_values = false;	// if we want "SINVM::_debug_values()" after execution
	bool produce_asm_file = false;
	bool include_builtins = true;

	// if we wrote to a stringstream
	bool saved_stringstream = false;
	std::stringstream sina_code;

	// wordsize will default to 16, but we can set it with the --wsxx flag
	uint8_t wordsize = 16;

	// our file name should be the zeroth element in the vector (syntax is "SIN file_name flags")
	std::string filename = program_arguments[0];
	std::string file_extension;
	std::string filename_no_extension;

	// a list of our dependencies
	std::vector<std::string> library_names;

	// get the file extension using "std::find"
	size_t extension_position = filename.find(".");
	if (extension_position != filename.npos) {
		file_extension = filename.substr(extension_position);	// using substr() we can get the extension from the position we just found
		// create a copy of the filename without an extension
		filename_no_extension = filename.substr(0, extension_position);
	}
	else {
		if (program_arguments[0] == "--help") {
			help();
			std::cout << "Press enter to exit..." << std::endl;
			std::cin.get();
			exit(0);
		}
		else {
			std::cerr << "**** First argument must be a filename." << std::endl;
			std::cerr << "Press enter to exit..." << std::endl;
			std::cin.get();
			exit(1);
		}
	}

	// iterate through the vector and set flags appropriately
	for (std::vector<std::string>::iterator arg_iter = program_arguments.begin(); arg_iter != program_arguments.end(); arg_iter++) {
		// only check for flags if the first character is '-'
		if ((*arg_iter)[0] == '-') {

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*i.*")) || (*arg_iter == "--interpret")) {
				interpret = true;
			}

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*c.*")) || (*arg_iter == "--compile")) {
				compile = true;
			}

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*s.*")) || (*arg_iter == "--assemble")) {
				assemble = true;
			}

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*d.*")) || (*arg_iter == "--disassemble")) {
				disassemble = true;
			}

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*l.*")) || (*arg_iter == "--link")) {
				link = true;
			}

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*e.*")) || (*arg_iter == "--execute")) {
				execute = true;
			}

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*a.*")) || (*arg_iter == "--produce-asm-file")) {
				produce_asm_file = true;
			}

			if (std::regex_match(*arg_iter, std::regex("-[a-zA-Z0-9]*h.*")) || (*arg_iter == "--help")) {
				help();
			}

			// if the parse-only flag is set
			if ((*arg_iter == "--parse-only")) {
				// overrides all other flags
				compile = false;
				assemble = false;
				disassemble = false;
				link = false;
				execute = false;
			}

			// if the compile-only flag is set, it will both produce an AST file and compile the code to SINASM
			if ((*arg_iter == "--compile-only")) {
				compile_only = true;
				compile = true;	// set only the compile flag; clear all others regardless if we set them or not
				assemble = false;
				disassemble = false;
				link = false;
				execute = false;
			}

			// if the debug flag is set
			if ((*arg_iter == "--debug")) {
				debug_values = true;
			}

			if ((*arg_iter == "--suppress-builtins")) {
				include_builtins = false;
			}

			// if we explicitly set word size
			if (std::regex_match(*arg_iter, std::regex("--ws.+", std::regex_constants::icase))) {
				std::string wordsize_string = arg_iter->substr(4);
				wordsize = (uint8_t)std::stoi(wordsize_string);
			}
		}
		// if we have a .sinc file as a parameter
		else if (std::regex_match(*arg_iter, std::regex("[a-zA-Z0-9_-]+\.sinc"))) {
			std::ifstream sinc_file;
			sinc_file.open(*arg_iter, std::ios::in | std::ios::binary);
			if (sinc_file.is_open()) {
				// initialize the object file
				SinObjectFile obj_file(sinc_file);
				objects_vector->push_back(obj_file);

				sinc_file.close();
			}
			else {
				file_error(filename);
				exit(1);
			}
		}
	}

	if (!compile_only && compile && !produce_asm_file) {
		assemble = true;
	}

	// Now that we have the flags all proper, we can execute things in the correct order
	// The functions are called in this order: compile, disassemble, assemble, link, execute

	try {
		// interpret a .sin file
		if (interpret) {
			throw std::runtime_error("**** Interpreted-SIN is currently not supported.");
		}
		// compile a .sin file
		if (compile) {
			// validate file type
			if (file_extension == ".sin") {
				// open the file
				std::ifstream sin_file;
				sin_file.open(filename, std::ios::in);

				if (sin_file.is_open()) {
					// create a vector of file names
					std::vector<std::string> object_file_names;
					
					// compile the file -- allocate the compiler object on the heap because it's a pretty big object
					Compiler* compiler = new Compiler(sin_file, wordsize, &object_file_names, &library_names, include_builtins);

					// if we want to produce an asm file
					if (produce_asm_file) {
						compiler->produce_sina_file(filename_no_extension + ".sina", include_builtins);

						// create SinObjectFile objects for each item in object_file_names
						for (std::vector<std::string>::iterator it = object_file_names.begin(); it != object_file_names.end(); it++) {
							// open the file of the name stored in object_file_names at the iterator's current position
							std::ifstream sinc_file;
							sinc_file.open(*it, std::ios::in | std::ios::binary);

							if (sinc_file.is_open()) {
								// if we can find the file, then create an object from it and add it to the object vector
								SinObjectFile obj(sinc_file);
								objects_vector->push_back(obj);
							}
							else {
								file_error(*it);
								exit(1);
							}

							// close the file before the next iteration
							sinc_file.close();
						}

						// update the filename
						file_extension = ".sina";
						filename = filename_no_extension + file_extension;
					}
					// if we just want to generate code and assemble it
					else {
						sina_code << compiler->compile_to_stringstream(include_builtins).str();
						saved_stringstream = true;

						// create object files for each object in object_file_names
						for (std::vector<std::string>::iterator it = object_file_names.begin(); it != object_file_names.end(); it++) {
							// open the file of the name stored in object_file_names at the iterator's current position
							std::ifstream sinc_file;
							sinc_file.open(*it, std::ios::in | std::ios::binary);

							if (sinc_file.is_open()) {
								// if we can find the file, then create an object from it and add it to the object vector
								SinObjectFile obj(sinc_file);
								objects_vector->push_back(obj);
							}
							else {
								file_error(*it);
								exit(1);
							}

							// close the file before the next iteration
							sinc_file.close();
						}
					}

					// close our file
					sin_file.close();

					// delete our compiler object
					delete compiler;

				} else {
					file_error(filename);
					exit(1);
				}
			}
			else {
				throw std::runtime_error("**** To compile, file type must be 'sin'.");
			}
		}
		
		// use an Assembler object for assembly and disassembly alike
		if (disassemble || assemble) {
			if (disassemble) {
				// validate file type
				if ((file_extension == ".sinc") || file_extension == ".sml") {
					std::ifstream to_disassemble;
					to_disassemble.open(filename, std::ios::in | std::ios::binary);
					if (to_disassemble.is_open()) {
						// initialize the assembler with the file and our "wordsize" variable, which defaults to 16
						Assembler disassembler(to_disassemble, wordsize);
						disassembler.disassemble(to_disassemble, filename_no_extension);

						// update the filename
						file_extension = ".sina";
						filename = filename_no_extension + file_extension;

						to_disassemble.close();
					}
					else {
						file_error(filename);
						exit(1);
					}
				}
				else {
					throw std::runtime_error("**** To disassemble, file type must be 'sinc' or 'sml'.");
				}
			}
			else if (assemble) {
				if ((compile && produce_asm_file) || !compile) {
					if (file_extension == ".sina") {
						std::ifstream sina_file;
						sina_file.open(filename, std::ios::in);
						if (sina_file.is_open()) {
							// write an object file using an Assembler and SinObjectFile objects
							Assembler* assemble = new Assembler(sina_file, wordsize);	// use the heap because it's a fairly large object
							assemble->create_sinc_file(filename_no_extension);

							// update our object files list
							std::vector<std::string> obj_filenames = assemble->get_obj_files_to_link();	// so we don't need to execute a function in every iteration of the loop

							for (std::vector<std::string>::iterator it = obj_filenames.begin(); it != obj_filenames.end(); it++) {
								std::ifstream sinc_file;
								sinc_file.open(*it, std::ios::in | std::ios::binary);
								SinObjectFile obj_file_from_assembler(sinc_file);

								if (sinc_file.is_open()) {
									// open the file
									obj_file_from_assembler.load_sinc_file(sinc_file);
									// add it to our objects vector
									objects_vector->push_back(obj_file_from_assembler);
									// close the file
									sinc_file.close();
								}
								else {
									throw std::runtime_error("**** Could not load object file '" + *it + "' after assembly.");
								}
							}

							// update the filename
							file_extension = ".sinc";
							filename = filename_no_extension + file_extension;

							sina_file.close();

							delete assemble;	// free the memory we allocated for our assembler
						}
						else {
							file_error(filename);
							exit(1);
						}
					}
					else {
						throw std::runtime_error("**** To assemble, file type must be 'sina'.");
					}
				}
				else if (compile && !produce_asm_file) {
					if (saved_stringstream) {
						// assemble the code in the stream
						Assembler assemble(sina_code, wordsize);
						assemble.create_sinc_file(filename_no_extension);

						// update our object files list
						std::vector<std::string> obj_filenames = assemble.get_obj_files_to_link();	// so we don't need to execute a function in every iteration of the loop
						for (std::vector<std::string>::iterator it = obj_filenames.begin(); it != obj_filenames.end(); it++) {
							// create the SinObjectFile object as well as the file stream for the sinc file
							SinObjectFile obj_file_from_assembler;
							std::ifstream sinc_file;
							sinc_file.open(*it, std::ios::in | std::ios::binary);

							if (sinc_file.is_open()) {
								// open the file
								obj_file_from_assembler.load_sinc_file(sinc_file);
								// add it to the front of our objects vector
								objects_vector->insert(objects_vector->begin(), obj_file_from_assembler);
								// close the file
								sinc_file.close();
							}
							else {
								throw std::runtime_error("**** Could not load object file '" + *it + "' after assembly.");
							}
						}

						// update the filename
						file_extension = ".sinc";
						filename = filename_no_extension + file_extension;
					}
					else {
						throw std::runtime_error("**** Stringstream was not saved!");
					}
				}
			}
		}

		// link files
		if (link) {
			// if we have object files to linke
			if (objects_vector->size() != 0) {
				// create a linker object using our objects vector
				Linker linker(*objects_vector);
				linker.create_sml_file(filename_no_extension);

				// update the filename
				file_extension = ".sml";
				filename = filename_no_extension + file_extension;
			}
			else {
				throw std::runtime_error("**** You must supply object files to link.");
			}
		}

		// execute a file
		if (execute) {
			// validate file extension
			if (file_extension == ".sml") {
				std::ifstream sml_file;
				sml_file.open(filename, std::ios::in | std::ios::binary);
				if (sml_file.is_open()) {
					// create an instance of the SINVM with our SML file and run it
					SINVM* vm = new SINVM(sml_file);	// use the heap because the vm is pretty large
					vm->run_program();

					if (debug_values) {
						vm->_debug_values();
						std::cout << "Done. Press enter to exit..." << std::endl;
						std::cin.get();
					}

					sml_file.close();

					delete vm;	// free the memory used by the vm
				}
				else {
					file_error(filename);
					exit(1);
				}

			}
			else {
				throw std::runtime_error("**** The SIN VM may only run SIN VM executable files (.sml).");
			}
		}
	}
	// if an exception was thrown, catch it
	catch (std::exception& e) {
		// print the error message
		std::cout << "\n\n********************" << std::endl;
		std::cout << "The program had to abort because the following exception occurred during execution:" << std::endl;
		std::cout << "\t" << e.what() << std::endl;
		std::cout << "\nPress enter to exit..." << std::endl;

		// wait for user input before exiting
		std::cin.get();

		// exit with code 2 (an execption occurred)
		exit(2);
	}

	// free the memory for objects_vector
	delete objects_vector;

	// exit the program
	return 0;
}