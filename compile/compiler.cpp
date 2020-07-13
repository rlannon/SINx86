/*

SIN Toolchain (x86 target)
compile.cpp

Implementation of the compiler class

Copyright 2019 Riley Lannon

*/

#include "compiler.h"

std::shared_ptr<symbol> compiler::lookup(std::string name, unsigned int line) {
    /*

    lookup
    Finds a symbol in the symbol table

    Looks up a symbol in the symbol table and returns it if found. Else, throws an exception.

    @param  name    The name of the symbol to find
    @param  line    The line where the lookup is needed
    @return A shared_ptr containing the symbol in question
    @throws Throws a SymbolNotFoundException if the symbol does not exist

    */

	std::shared_ptr<symbol> to_return;

	try {
		to_return = this->symbols.find(name);
	}
	catch (std::exception e) {
		throw SymbolNotFoundException(line);
	}

	return to_return;
}

// we need to specify which classes can be used for our <typename T> since it's implemented in a separate file
template void compiler::add_symbol(symbol&, unsigned int);
template void compiler::add_symbol(function_symbol&, unsigned int);

template<typename T>
void compiler::add_symbol(T &to_add, unsigned int line) {
    /*

    add_symbol
    Adds a symbol to the table

    Adds a symbol to the symbol table, throwing an exception if it's a duplicate.
    Since this is a template function, it can handle either symbols or function symbols. And, since the symbol table uses shared pointers, truncation won't be an issue.
	Note this function checks to see if the symbol name begins with "sinl_", the prefix for SIN Runtime Environment functions and data. If it does, the compiler will issue a warning stating that errors may be encountered at link time, but it will continue compilation.

    @param  to_add  The symbol we want to add
    @param  line    The line number where the allocation occurs

    @throws DuplicateSymbolException if T is class 'symbol' and it couldn't be added
	@throws	DuplicateDefinitionException if T is class 'function_symbol' and it couldn't be added

    */

	// check for sinl_ prefix
	// size_t pos = to_add.get_name().find("sinl_");
	// if (pos != std::string::npos && pos == 0) {
	// 	compiler_warning("'sinl_' is a reserved prefix for SIN runtime environment symbols. Using this prefix may result in link-time errors due to multiple symbol definition.");
	// }

	// insert the symbol
    std::shared_ptr<T> s = std::make_shared<T>(to_add);
    bool ok = this->symbols.insert(s);

    // if the symbol could not be inserted, we *might* need to throw an exception
    // it's also possible the symbol was added as a declaration and is now being defined
    if (!ok) {
        // get the current symbol
        auto sym = this->symbols.find(s->get_name());

        // if it was defined, throw an error
        if (sym->is_defined()) {
            // if it's a function we are adding, throw a duplicate *definition* exception; else, it's a duplicate symbol
            if (to_add.get_symbol_type() == SymbolType::FUNCTION_SYMBOL)
                throw DuplicateDefinitionException(line);
            else
                throw DuplicateSymbolException(line);
        }
        // otherwise, mark the symbol as defined
        else {
            sym->set_defined();
        }
    }
}

void compiler::add_struct(struct_info to_add, unsigned int line) {
	/*
	
	add_struct
	Adds a struct to the compiler's struct table

	@param	to_add	The struct_info object we need to put in the struct table (contains information about the struct)
	@param	line	The line where the definition occurs

	@throws	DuplicateDefinitionException if the struct couldn't be added
	
	*/

	// todo: can this function and add_symbol utilize templates to be combined into one function?

	bool ok = this->structs.insert(to_add);

    // if the struct was defined, throw an exception; otherwise, mark it as defined and update the struct_info object
	if (!ok) {
        // if the width is known, it was already defined
        auto s_info = this->structs.find(to_add.get_struct_name());
        if (s_info.is_width_known()) {
    		throw DuplicateDefinitionException(line);
        }
        else {
            s_info = to_add;
        }
    }
}

struct_info& compiler::get_struct_info(std::string struct_name, unsigned int line) {
    /*
    
    get_struct_info
    Looks up a struct with the given name in the struct table

    @param  struct_name The name of the struct to find
    @param  line    The line where the lookup occurs; necessary in case this function throws an exception
    @return A struct_info object containing the information
    @throws Throws an UndefinedException if the struct is not known

    */
	
	// check to see whether our struct is in the table first
	if (!this->structs.contains(struct_name)) {
		throw UndefinedException(line);
	}

	return this->structs.find(struct_name);
}

std::stringstream compiler::compile_statement(std::shared_ptr<Statement> s, std::shared_ptr<function_symbol> signature) {
    /*

        Compiles a single statement to x86, dispatching appropriately

    */

    std::stringstream compile_ss;

    // todo: set-up functionality?

    // The statement will be casted to the appropriate type and dispatched
    stmt_type s_type = s->get_statement_type();
    switch (s_type) {
        case INCLUDE:
        {
            // includes must be in the global scope at level 0
            if (this->current_scope_name == "global" && this->current_scope_level == 0) {
                // Included files will not be added more than once in any compilation process -- so we don't need anything like "pragma once"
                auto include = dynamic_cast<Include*>(s.get());
                compile_ss << this->process_include(include->get_filename(), include->get_line_number()).str();
            }
            else {
                throw CompilerException(
                    "Include statements must be made in the global scope at level 0",
                    compiler_errors::INCLUDE_SCOPE_ERROR,
                    s->get_line_number()
                );
            }

            break;
        }
        case DECLARATION:
        {
            // handle a declaration
            Declaration *decl_stmt = dynamic_cast<Declaration*>(s.get());

            // we need to ensure that the current scope is global -- declarations can only happen in the global scope, as they must be static
            if (this->current_scope_name == "global" && this->current_scope_level == 0) {
                compile_ss << this->handle_declaration(*decl_stmt).str();
            } else {
                throw DeclarationException(decl_stmt->get_line_number());
            }
            break;
        }
        case ALLOCATION:
        {
            Allocation *alloc_stmt = dynamic_cast<Allocation*>(s.get());
            compile_ss << this->allocate(*alloc_stmt).str() << std::endl;
            break;
        }
        case ASSIGNMENT:
        {
            Assignment *assign_stmt = dynamic_cast<Assignment*>(s.get());
            compile_ss << this->handle_assignment(*assign_stmt).str() << std::endl;
            break;
        }
        case RETURN_STATEMENT:
        {
            // return statements may only occur within functions; if 'signature' wasn't passed to this function, then we aren't compiling code inside a function and must throw an exception
            if (signature) {
                ReturnStatement *return_stmt = dynamic_cast<ReturnStatement*>(s.get());
                compile_ss << this->handle_return(*return_stmt, *(signature.get())).str() << std::endl;
            } else {
                throw IllegalReturnException(s->get_line_number());
            }
            break;
        }
        case IF_THEN_ELSE:
		{
			// first, we need to cast and get the current block number (in case we have nested blocks)
			IfThenElse *ite = dynamic_cast<IfThenElse*>(s.get());
			size_t current_scope_num = this->scope_block_num;
			this->scope_block_num += 1; // increment the scope number now in case we have recursive conditionals
			
			// then we need to evaluate the expression; if the final result is 'true', we continue in the tree; else, we branch to 'else'
			// if there is no else statement, it falls through to 'done'
			compile_ss << this->evaluate_expression(ite->get_condition(), ite->get_line_number()).str();
            compile_ss << "\t" << "cmp al, 1" << std::endl;
            compile_ss << "\t" << "jne sinl_ite_else_" << current_scope_num << std::endl;	// compare the result of RAX with 0; if true, then the condition was false, and we should jump
			
			// compile the branch
			compile_ss << this->compile_statement(ite->get_if_branch(), signature).str();

			// now, we need to jump to "done" to ensure the "else" branch is not automatically executed
			compile_ss << "\t" << "jmp sinl_ite_done_" << current_scope_num << std::endl;
			compile_ss << "sinl_ite_else_" << current_scope_num << ":" << std::endl;
            
			// compile the branch, if one exists
			if (ite->get_else_branch().get()) {
				compile_ss << this->compile_statement(ite->get_else_branch(), signature).str();
			}

			// clean-up
			compile_ss << "sinl_ite_done_" << current_scope_num << ":" << std::endl;
			break;
		}
		case WHILE_LOOP:
        {
            WhileLoop *while_stmt = dynamic_cast<WhileLoop*>(s.get());
            
            // create a loop heading, evaluate the condition
            auto current_block_num = this->scope_block_num;
            this->scope_block_num += 1;

            compile_ss << "sinl_while_" << current_block_num << ":" << std::endl;
            compile_ss << this->evaluate_expression(while_stmt->get_condition(), while_stmt->get_line_number()).str();
            compile_ss << "\t" << "cmp al, 1" << std::endl;
            compile_ss << "\t" << "jne sinl_while_done_" << current_block_num << std::endl;

            // compile the loop body
            compile_ss << this->compile_statement(while_stmt->get_branch(), signature).str();
            compile_ss << "\t" << "jmp sinl_while_" << current_block_num << std::endl;

            compile_ss << "sinl_while_done_" << current_block_num << ":" << std::endl;
            break;
        }
        case FUNCTION_DEFINITION:
        {
            FunctionDefinition *def_stmt = dynamic_cast<FunctionDefinition*>(s.get());

			// ensure the function has a return value in all control paths
			if (general_utilities::returns(*def_stmt->get_procedure().get())) {
                if (def_stmt->get_calling_convention() == SINCALL) {
                    compile_ss << this->define_function(*def_stmt).str() << std::endl;
                } else {
                    throw CompilerException(
                        "Currently, defining non-sincall functions is not supported",
                        compiler_errors::CALLING_CONVENTION_ERROR,
                        def_stmt->get_line_number()
                    );
                }
            } else {
                throw NoReturnException(s->get_line_number());
            }
            break;
        }
        case STRUCT_DEFINITION:
		{
			StructDefinition *def_stmt = dynamic_cast<StructDefinition*>(s.get());

			struct_info defined = define_struct(*def_stmt, this->evaluator);
			this->add_struct(defined, s->get_line_number());

			break;
		}
        case CALL:
        {
            Call *call_stmt = dynamic_cast<Call*>(s.get());
            compile_ss << this->call_function(*call_stmt, call_stmt->get_line_number()).str() << std::endl;
            break;
        }
        case INLINE_ASM:
        {
            // writes asm directly to file
            // warns user that this is very unsafe
            compiler_warning(
                "Use of inline assembly is highly discouraged as it cannot be analyzed by the compiler nor utilize certain runtime safety measures (unless done manually)",
                compiler_errors::UNSAFE_OPERATION,
                s->get_line_number()
            );
            InlineAssembly *asm_stmt = dynamic_cast<InlineAssembly*>(s.get());

            // todo: write asm to file

            break;
        }
        case FREE_MEMORY:
            /*

            'free' may be used with either automatic or dynamic memory -- it may not be used with const or static
            however, it has very little effect on automatic memory; it just marks the memory as freed, preventing any future writes to it

			'free' is also safe in that it will not trigger a fault if you call 'free' on the same data twice
            (though if the compiler sees this happen, then it will generate a warning)

            */

            break;
        case SCOPE_BLOCK:
        {
            /*

            Scope blocks can be treated as individual statements in certain cases

            */

            ScopedBlock *block = dynamic_cast<ScopedBlock*>(s.get());
            StatementBlock ast = block->get_statements();

            // be sure to adjust scope levels
            unsigned int old_scope_level = this->current_scope_level;
            this->current_scope_level += 1;
            
            // compile the AST in the block
            compile_ss << this->compile_ast(ast, signature).str();

            // free local data
            compile_ss << decrement_rc(this->reg_stack.peek(), this->symbols, this->current_scope_name, this->current_scope_level, false).str();
            
            // restore the scope level
            this->current_scope_level = old_scope_level;
            break;
        }
        default:
            break;
    };

    // todo: any clean-up should go here

    // finally, return the compiled code
    return compile_ss;
}

std::stringstream compiler::compile_ast(StatementBlock &ast, std::shared_ptr<function_symbol> signature) {
    /*

    compile_ast
    Compiles a StatementBlock and returns the generated code
    
    Iterates over all the statements in the AST, generating the code statement by statement. This will naturally utilize recursion to compile other functions, etc.

    @param  ast The AST for which we are generating code
    @return A stringstream containing the generated code

    */

    std::stringstream compile_ss;

    // iterate over it and compile each statement in turn, adding it to the stringstream
    for (std::shared_ptr<Statement> s: ast.statements_list) {
        compile_ss << this->compile_statement(s, signature).str();
    }
    
    // todo: free data for non-function scope blocks

	// when we leave a scope, remove local variables -- but NOT global variables (they must be retained for inclusions)
	if (this->current_scope_name != "global") {
		// todo: call leave_scope on the compile-time evaluator
		this->symbols.leave_scope(this->current_scope_name, this->current_scope_level);
	}

    return compile_ss;
}

std::stringstream compiler::process_include(std::string include_filename, unsigned int line) {
    /*

    process_include
    Processes an include statement, returning any code that was generated

    See docs/Includes.md for more information on how includes are handled.

    */

    std::stringstream include_ss;

    // adjust the path
    if (include_filename.length() > 0 && include_filename[0] != '~' && include_filename[0] != '/') {
        include_filename = this->file_path + include_filename;
    }

    // check to see if this file was already included (with the proper path)
    if (this->compiled_headers.count(include_filename)) {
        compiler_note(
            "Included file \"" + include_filename + "\" will be ignored here, as it has been included elsewhere",
            line
        );
    }
    // if not, compile it
    else {
        // create the AST
        auto sin_parser = new Parser(include_filename);
        StatementBlock ast = sin_parser->create_ast();
        delete sin_parser;

        // walk through the AST and handle relevant statements
        for (std::shared_ptr<Statement> s: ast.statements_list) {
            if (s->get_statement_type() == ALLOCATION) {
                auto a = dynamic_cast<Allocation*>(s.get());

                // allocations must be qualified with 'extern'
                if (a->get_type_information().get_qualities().is_extern()) {
                    // add the symbol
                    auto sym = generate_symbol(
                        *a,
                        a->get_type_information().get_width(),
                        "global",
                        0,
                        this->max_offset,
                        false
                    );
                    this->add_symbol(sym, a->get_line_number());
                }
                else {
                    throw InvisibleSymbolException(a->get_line_number());
                }
            }
            else if (s->get_statement_type() == FUNCTION_DEFINITION) {
                auto f = dynamic_cast<FunctionDefinition*>(s.get());

                // function definitions must be 'extern'
                if (f->get_type_information().get_qualities().is_extern()) {
                    // create the function symbol
                    auto sym = create_function_symbol(
                        *f,
                        false
                    );
                    this->add_symbol(sym, f->get_line_number());
                }
                else {
                    throw InvisibleSymbolException(f->get_line_number());
                }
            }
            else if (s->get_statement_type() == STRUCT_DEFINITION) {
                // included struct definitions
                auto d = dynamic_cast<StructDefinition*>(s.get());
                struct_info s_info = define_struct(*d, this->evaluator);
                this->add_struct(s_info, d->get_line_number());
            }
            else if (s->get_statement_type() == DECLARATION) {
                auto d = dynamic_cast<Declaration*>(s.get());
                include_ss << this->handle_declaration(*d).str();
            }
            else if (s->get_statement_type() == INCLUDE) {
                auto inc = dynamic_cast<Include*>(s.get());
                include_ss << this->process_include(inc->get_filename(), inc->get_line_number()).str();
            }
            else {
                // ignore all other statements
                continue;
            }
        }

        // mark the file as included (with the proper path)
        this->compiled_headers.insert(include_filename);
    }

    return include_ss;
}

void compiler::generate_asm(std::string filename) {
    /*

    generate_asm
    The compiler's entry function

    Generates x86 code for the program, in NASM syntax, and writes it to a file.
    The outfile will have the same name as the .sin file being compiled, the exception being a .s extension instead of .sin
    Note the parser supplied will generate the AST here

    @param  filename    The name of the file we wish to compile
    @param  p   The parser object that will be used to parse the file

    */

    // catch parser exceptions here
    try {
        this->filename = filename;

        // all include paths should be relative to the path of the file being compiled, unless a / or ~ is at the beginning
        size_t last_slash = filename.find_last_of("/");
        if (last_slash != std::string::npos)
            this->file_path = filename.substr(0, last_slash+1);
        else
            this->file_path = "";

        // create our abstract syntax tree
        std::cout << "Compiling " << filename << std::endl;
		auto sin_parser = new Parser(filename);
        std::cout << "Parsing..." << std::endl;
        StatementBlock ast = sin_parser->create_ast();
        delete sin_parser;

        // The code we are generating will go in the text segment -- writes to the data and bss sections will be done as needed in other functions
		std::cout << "Generating code..." << std::endl;
        this->text_segment << "%ifndef _SRE_INCLUDE_" << std::endl;
        this->text_segment << "%define _SRE_INCLUDE_" << std::endl;
        this->text_segment << "%include \"../SRE/src/asm/asm_include.s\"" << std::endl; // todo: better linkage to SRE
        this->text_segment << "%endif" << std::endl;
        this->text_segment << "default rel" << std::endl;   // use 'default rel' to ensure we have PIC
		this->text_segment << this->compile_ast(ast).str();

		std::cout << "Consolidating code..." << std::endl;
        // now, we want to see if we have a function 'main' in the symbol table; if so, we need to set it up and call it
        try {
            // if we have a main function in this file, then insert our entry point (set up stack frame and call main)
            std::shared_ptr<symbol> main_function = this->lookup("main", 0);
            function_symbol main_symbol = *dynamic_cast<function_symbol*>(main_function.get());
            
            // 'main' should have a return type of 'int'; if not, issue a warning
            if (main_function->get_data_type().get_primary() != INT) {
                compiler_warning(
                    "Function 'main' should have a return type of 'int'",
                    compiler_errors::MAIN_SIGNATURE,
                    main_function->get_line_defined()
                );
            }

            // check parameters; should have one with type 'dynamic array<string>'
            if (main_symbol.get_formal_parameters().size() != 1) {
                compiler_warning(
                    "Function 'main' should include one argument, 'dynamic array<string> args'",
                    compiler_errors::MAIN_SIGNATURE,
                    main_function->get_line_defined()
                );
            }
            
            // todo: get actual command-line arguments, convert them into SIN data types
            std::vector<std::shared_ptr<Expression>> cmd_args = {};
            for (symbol s: main_symbol.get_formal_parameters()) {
                // todo: get argument
            }

            // add 'extern' for every symbol that needs it
            for (std::string s: this->externals) {
                this->text_segment << "extern " << s << std::endl;
            }

            // insert our wrapper for the program
            this->text_segment << "global main" << std::endl;
            this->text_segment << "main:" << std::endl;

            // call SRE init function (takes no parameters)
            this->text_segment << "\t" << "mov rax, 0" << std::endl;
            this->text_segment << "\t" << "call sre_init" << std::endl;

            // call the main function with SINCALL
            this->text_segment << this->sincall(main_symbol, cmd_args, 0).str();

            // preserve the return value and call SRE cleanup function
            this->text_segment << "\t" << "push rax" << std::endl;
            this->text_segment << "\t" << "call sre_clean" << std::endl;

            // restore main's return value and return
            this->text_segment << "\t" << "pop rax" << std::endl;
            this->text_segment << "\t" << "ret" << std::endl;
        } catch (SymbolNotFoundException &e) {
            // print a warning saying no entry point was found -- but SIN files do not have to have entry points, as they might be included
            compiler_note("No entry point found in file \"" + filename + "\"", 0);
        }

        // remove the extension from the file name
        size_t last_index = filename.find_last_of(".");
        if (last_index != std::string::npos)
            filename = filename.substr(0, last_index);
        
        // append ".s", the extension
        filename += ".s";
        
        // now, save text, data, and bss segments to our outfile
        std::ofstream outfile;
        outfile.open(filename, std::ios::out);

        // first, write the text section
        outfile << "section .text" << std::endl;
        outfile << this->text_segment.str() << std::endl;

		// next, the .rodata
		outfile << "section .rodata" << std::endl;
		outfile << "\t" << "sp_mask dd 0x80000000" << std::endl;	// we have bitmasks for single- and double-precision floats; they should be read-only
		outfile << "\t" << "dp_mask dq 0x8000000000000000" << std::endl;	// todo: do we really need this? or is there an easier way to flip the sign?
		outfile << this->rodata_segment.str() << std::endl;

        // next, the .data section
        outfile << "section .data" << std::endl;
        outfile << this->data_segment.str() << std::endl;

        // finally, the .bss section
        outfile << "section .bss" << std::endl;
        outfile << this->bss_segment.str() << std::endl;

		// print a message when we are done
		std::cout << "Done." << std::endl;

        // close the outfile
        outfile.close();

		// print a message saying compilation has finished
		std::cout << "Compilation finished successfully." << std::endl;
    } catch (std::exception &e) {
        // todo: exception handling should be improved
        std::cout << "An error occurred during compilation:" << std::endl;
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
    // initialize our number trackers
    this->strc_num = 0;
    this->strcmp_num = 0;
    this->fltc_num = 0;
    this->rtbounds_num = 0;
    this->list_literal_num = 0;
    this->scope_block_num = 0;
    this->max_offset = 8;   // should be 8 (a qword) because of the way the x86 stack works
    
    // initialize the scope
    this->current_scope_name = "global";
    this->current_scope_level = 0;

    // initialize the compile-time evaluator
    this->evaluator = compile_time_evaluator(&this->structs);
}

compiler::~compiler() {
}
