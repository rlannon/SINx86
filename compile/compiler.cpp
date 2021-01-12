/*

SIN Toolchain (x86 target)
compile.cpp

Implementation of the compiler class

Copyright 2019 Riley Lannon

*/

#include "compiler.h"
#include "compile_util/function_util.h"

symbol *compiler::lookup(std::string name, unsigned int line) {
    /*

    lookup
    Finds a symbol in the symbol table

    Looks up a symbol in the symbol table and returns it if found. Else, throws an exception.

    @param  name    The name of the symbol to find
    @param  line    The line where the lookup is needed
    @return A shared_ptr containing the symbol in question
    @throws Throws a SymbolNotFoundException if the symbol does not exist

    */

	symbol *to_return = nullptr;

	try {
		to_return = &this->symbols.find(name);
	}
	catch (SymbolNotFoundException &e) {
        e.set_line(line);
        throw e;
	}

	return to_return;
}

symbol &compiler::add_symbol(symbol &to_add, unsigned int line) {
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

	// insert the symbol
    std::shared_ptr<symbol> s = nullptr;
    if (to_add.get_symbol_type() == VARIABLE) {
        s = std::make_shared<symbol>(to_add);
    }
    else if (to_add.get_symbol_type() == FUNCTION_SYMBOL) {
        function_symbol &fs = static_cast<function_symbol&>(to_add);
        s = std::make_shared<function_symbol>(fs);
    }
    else {
        throw InvalidSymbolException(line);
    }
    
    return this->add_symbol(s, line);
}

symbol &compiler::add_symbol(std::shared_ptr<symbol> to_add, unsigned int line) {
    /*

    add_symbol
    An overloaded version to add a symbol when we already have a shared pointer to it

    */

    symbol *inserted = this->symbols.insert(to_add);

    // if the symbol could not be inserted, we *might* need to throw an exception
    // it's also possible the symbol was added as a declaration and is now being defined
    if (!inserted) {
        // get the current symbol
        auto &sym = this->symbols.find(to_add->get_name());

        // if it was defined, throw an error
        if (sym.is_defined()) {
            // if it's a function we are adding, throw a duplicate *definition* exception; else, it's a duplicate symbol
            if (to_add->get_symbol_type() == SymbolType::FUNCTION_SYMBOL)
                throw DuplicateDefinitionException(line);
            else
                throw DuplicateSymbolException(line);
        }
        // otherwise, mark the symbol as defined
        else {
            sym.set_defined();
        }

        return sym;
    }
    else {
        return *inserted;
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
        auto s_info = this->structs.find(to_add.get_struct_name(), line);
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

    // todo: delete this function
	
	// check to see whether our struct is in the table first
	return this->structs.find(struct_name, line);
}

std::stringstream compiler::compile_statement(Statement &s, function_symbol *signature) {
    /*

        Compiles a single statement to x86, dispatching appropriately

    */

    std::stringstream compile_ss;

    // todo: set-up functionality?

    // The statement will be casted to the appropriate type and dispatched
    stmt_type s_type = s.get_statement_type();
    switch (s_type) {
        case INCLUDE:
        {
            // includes must be in the global scope at level 0
            if (this->current_scope_name == "global" && this->current_scope_level == 0) {
                // Included files will not be added more than once in any compilation process -- so we don't need anything like "pragma once"
                auto include = static_cast<Include&>(s);
                compile_ss << this->process_include(include.get_filename(), include.get_line_number()).str();
            }
            else {
                throw CompilerException(
                    "Include statements must be made in the global scope at level 0",
                    compiler_errors::INCLUDE_SCOPE_ERROR,
                    s.get_line_number()
                );
            }

            break;
        }
        case DECLARATION:
        {
            // handle a declaration
            auto &decl_stmt = static_cast<Declaration&>(s);

            // we need to ensure that the current scope is global -- declarations can only happen in the global scope, as they must be static
            if (this->current_scope_name == "global" && this->current_scope_level == 0) {
                compile_ss << this->handle_declaration(decl_stmt).str();
            } else {
                throw DeclarationException(decl_stmt.get_line_number());
            }
            break;
        }
        case ALLOCATION:
        {
            auto &alloc_stmt = static_cast<Allocation&>(s);
            compile_ss << this->allocate(alloc_stmt).str() << std::endl;
            break;
        }
        case MOVEMENT:
        {
            auto &move_stmt = static_cast<Movement&>(s);
            compile_ss << this->handle_move(move_stmt).str() << std::endl;
            break;
        }
        case ASSIGNMENT:
        {
            auto &assign_stmt = static_cast<Assignment&>(s);
            compile_ss << this->handle_assignment(assign_stmt).str() << std::endl;
            break;
        }
        case RETURN_STATEMENT:
        {
            // return statements may only occur within functions; if 'signature' wasn't passed to this function, then we aren't compiling code inside a function and must throw an exception
            if (signature) {
                auto &return_stmt = static_cast<ReturnStatement&>(s);
                compile_ss << this->handle_return(return_stmt, *signature).str() << std::endl;
            } else {
                throw IllegalReturnException(s.get_line_number());
            }
            break;
        }
        case IF_THEN_ELSE:
		{
			// first, we need to cast and get the current block number (in case we have nested blocks)
			auto &ite = static_cast<IfThenElse&>(s);
			size_t current_scope_num = this->scope_block_num;
			this->scope_block_num += 1; // increment the scope number now in case we have recursive conditionals
			
			// then we need to evaluate the expression; if the final result is 'true', we continue in the tree; else, we branch to 'else'
			// if there is no else statement, it falls through to 'done'
            auto condition_p = this->evaluate_expression(ite.get_condition(), ite.get_line_number());
			compile_ss << condition_p.first;
            // todo: count
            
            compile_ss << "\t" << "cmp al, 1" << std::endl;
            compile_ss << "\t" << "jne " << magic_numbers::ITE_ELSE_LABEL << current_scope_num << std::endl;	// compare the result of RAX with 0; if true, then the condition was false, and we should jump
			
			// compile the branch
			compile_ss << this->compile_statement(*ite.get_if_branch(), signature).str();

			// now, we need to jump to "done" to ensure the "else" branch is not automatically executed
			compile_ss << "\t" << "jmp " << magic_numbers::ITE_DONE_LABEL << current_scope_num << std::endl;
			compile_ss << magic_numbers::ITE_ELSE_LABEL << current_scope_num << ":" << std::endl;
            
			// compile the branch, if one exists
			if (ite.get_else_branch()) {
				compile_ss << this->compile_statement(*ite.get_else_branch(), signature).str();
			}

			// clean-up
			compile_ss << magic_numbers::ITE_DONE_LABEL << current_scope_num << ":" << std::endl;
			break;
		}
		case WHILE_LOOP:
        {
            auto &while_stmt = static_cast<WhileLoop&>(s);
            
            // create a loop heading, evaluate the condition
            auto current_block_num = this->scope_block_num;
            this->scope_block_num += 1;
            auto condition_p = this->evaluate_expression(while_stmt.get_condition(), while_stmt.get_line_number());

            compile_ss << magic_numbers::WHILE_LABEL << current_block_num << ":" << std::endl;
            compile_ss << condition_p.first;
            // todo: count
            compile_ss << "\t" << "cmp al, 1" << std::endl;
            compile_ss << "\t" << "jne " << magic_numbers::WHILE_DONE_LABEL << current_block_num << std::endl;

            // compile the loop body
            compile_ss << this->compile_statement(*while_stmt.get_branch(), signature).str();
            compile_ss << "\t" << "jmp " << magic_numbers::WHILE_LABEL << current_block_num << std::endl;

            compile_ss << magic_numbers::WHILE_DONE_LABEL << current_block_num << ":" << std::endl;
            break;
        }
        case FUNCTION_DEFINITION:
        {
            auto &def_stmt = static_cast<FunctionDefinition&>(s);

			// ensure the function has a return value in all control paths
			if (general_utilities::returns(def_stmt.get_procedure())) {
                if (def_stmt.get_calling_convention() == SINCALL) {
                    compile_ss << this->define_function(def_stmt).str() << std::endl;
                } else {
                    throw CompilerException(
                        "Currently, defining non-sincall functions is not supported",
                        compiler_errors::UNSUPPORTED_FEATURE,
                        def_stmt.get_line_number()
                    );
                }
            } else {
                throw NoReturnException(s.get_line_number());
            }
            break;
        }
        case STRUCT_DEFINITION:
		{
			auto &def_stmt = static_cast<StructDefinition&>(s);

            // first, define the struct
			struct_info defined = define_struct(def_stmt, this->evaluator);
            
            // now we can add the struct to the table
			this->add_struct(defined, s.get_line_number());

            // now, if we had any member functions, we have to define them as well
            // update the scope name
            auto prev_name = this->current_scope_name;
            auto prev_level = this->current_scope_level;
            this->current_scope_name = defined.get_struct_name();
            this->current_scope_level += 1;

            for (auto member_s: def_stmt.get_procedure().statements_list) {
                if (member_s->get_statement_type() == FUNCTION_DEFINITION) {
                    auto func_def = static_cast<FunctionDefinition*>(member_s.get());
                    auto func_sym = defined.get_member(func_def->get_name());
                    if (func_sym->get_symbol_type() == FUNCTION_SYMBOL) {
                        function_symbol *f = static_cast<function_symbol*>(func_sym);
                        compile_ss << this->define_function(
                            *f,
                            func_def->get_procedure(),
                            func_def->get_line_number()
                        ).str();
                    }
                    else {
                        throw CompilerException(
                            "Expected to find member function",
                            compiler_errors::INVALID_SYMBOL_TYPE_ERROR,
                            func_def->get_line_number()
                        );
                    }
                }
            }

            // restore the scope name
            this->current_scope_name = prev_name;
            this->current_scope_level = prev_level;
            
			break;
		}
        case CALL:
        {
            auto &call_stmt = static_cast<Call&>(s);
            compile_ss << this->call_function(call_stmt, call_stmt.get_line_number()).first << std::endl;
            break;
        }
        case INLINE_ASM:
        {
            // writes asm directly to file
            // warns user that this is very unsafe
            compiler_warning(
                "Use of inline assembly is highly discouraged as it cannot be analyzed by the compiler nor utilize certain runtime safety measures (unless done manually)",
                compiler_errors::UNSAFE_OPERATION,
                s.get_line_number()
            );
            auto &asm_stmt = static_cast<InlineAssembly&>(s);

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

            auto &block = static_cast<ScopedBlock&>(s);
            StatementBlock ast = block.get_statements();

            // be sure to adjust scope levels
            unsigned int old_scope_level = this->current_scope_level;
            this->current_scope_level += 1;
            
            // compile the AST in the block
            compile_ss << this->compile_ast(ast, signature).str();

            // free local data
            compile_ss << decrement_rc(
                this->reg_stack.peek(),
                this->symbols,
                this->structs,
                this->current_scope_name,
                this->current_scope_level,
                false
            );
            
            // restore the scope level
            this->current_scope_level = old_scope_level;
            break;
        }
        default:
            throw CompilerException("This statement type is not currently supported", compiler_errors::ILLEGAL_OPERATION_ERROR, s.get_line_number());
            break;
    };

    // todo: any clean-up should go here

    // finally, return the compiled code
    return compile_ss;
}

std::stringstream compiler::compile_ast(StatementBlock &ast, function_symbol *signature) {
    /*

    compile_ast
    Compiles a StatementBlock and returns the generated code
    
    Iterates over all the statements in the AST, generating the code statement by statement. This will naturally utilize recursion to compile other functions, etc.

    @param  ast The AST for which we are generating code
    @return A stringstream containing the generated code

    */

    std::stringstream compile_ss;

    // iterate over it and compile each statement in turn, adding it to the stringstream
    for (auto s: ast.statements_list) {
        compile_ss << this->compile_statement(*s.get(), signature).str();
    }

	// when we leave a scope, remove local variables -- but NOT global variables (they must be retained for inclusions)
	if (this->current_scope_level != 0) {
		// todo: call leave_scope on the compile-time evaluator

        // free local data
        size_t reserved_space = this->symbols.leave_scope(this->current_scope_name, this->current_scope_level);

        // note we don't need to do have an "add rsp" instruction if we just had a return statement (it's unreachable)
        if ((this->current_scope_level != 1) && (ast.statements_list.back()->get_statement_type() != RETURN_STATEMENT)) {
            compile_ss << "\t" << "add rsp, " << reserved_space << std::endl;
            this->max_offset -= reserved_space;
        }
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
                auto a = static_cast<Allocation*>(s.get());

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
                auto f = static_cast<FunctionDefinition*>(s.get());

                // function definitions must be 'extern'
                if (f->get_type_information().get_qualities().is_extern()) {
                    // create the function symbol
                    auto sym = function_util::create_function_symbol(
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
                auto d = static_cast<StructDefinition*>(s.get());
                struct_info s_info = define_struct(*d, this->evaluator);
                this->add_struct(s_info, d->get_line_number());
            }
            else if (s->get_statement_type() == DECLARATION) {
                auto d = static_cast<Declaration*>(s.get());
                include_ss << this->handle_declaration(*d).str();
            }
            else if (s->get_statement_type() == INCLUDE) {
                auto inc = static_cast<Include*>(s.get());
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

        // add 'extern' for every symbol that needs it
        for (std::string s: this->externals) {
            this->text_segment << "extern " << s << std::endl;
        }

        // now, we want to see if we have a function 'main' in the symbol table; if so, we need to set it up and call it
        symbol *main_function = nullptr;

        try {
            main_function = this->lookup("main", 0);
        }
        catch (SymbolNotFoundException &e) {
            // print a warning saying no entry point was found -- but SIN files do not have to have entry points, as they might be included
            compiler_note("No entry point found in file \"" + filename + "\"", 0);
            main_function = nullptr;
        }

        // if we have a main function in this file, then insert our entry point (set up stack frame and call main)
        // if main is not a function symbol, issue a warning
        if (main_function && main_function->get_symbol_type() == FUNCTION_SYMBOL) {
            function_symbol &main_symbol = static_cast<function_symbol&>(*main_function);
            
            // 'main' should have a return type of 'int'; if not, issue a warning
            if (main_symbol.get_data_type().get_primary() != INT) {
                compiler_warning(
                    "Function 'main' should have a return type of 'int'",
                    compiler_errors::MAIN_SIGNATURE,
                    main_function->get_line_defined()
                );
            }

            // check parameters; should have one with type 'dynamic array<string>'
            if (main_symbol.get_formal_parameters().size() != 1) {
                throw CompilerException(
                    "Function 'main' should include one argument, 'dynamic array<string> args'",
                    compiler_errors::MAIN_SIGNATURE,
                    main_function->get_line_defined()
                );
            }
            else {
                auto cl_param = main_symbol.get_formal_parameters().at(0);
                if (
                    (cl_param->get_data_type().get_primary() != ARRAY) ||
                    (cl_param->get_data_type().get_subtype() != STRING) ||
                    !cl_param->get_data_type().get_qualities().is_dynamic()
                ) {
                    throw CompilerException(
                        "Function 'main' should include one argument, 'dynamic array<string> args'",
                        compiler_errors::MAIN_SIGNATURE,
                        main_function->get_line_defined()
                    );
                }
            }

            // insert our wrapper for the program
            this->text_segment << "global " << magic_numbers::MAIN_LABEL << std::endl;
            this->text_segment << magic_numbers::MAIN_LABEL << ":" << std::endl;

            // preserve argc and argv
            this->text_segment << "\t" << "mov r12, rdi" << std::endl
                << "\t" << "mov r13, rsi" << std::endl;

            // call SRE init function (takes no parameters) -- ensure 16-byte stack alignment
            this->text_segment << "\t" << "mov rax, rsp" << std::endl
                << "\t" << "and rsp, -0x10" << std::endl
                << "\t" << "push rax" << std::endl
                << "\t" << "sub rsp, 8" << std::endl
                << "\t" << "mov rax, 0" << std::endl
                << "\t" << "call " << magic_numbers::SRE_INIT << std::endl
                << "\t" << "add rsp, 8" << std::endl
                << "\t" << "pop rsp" << std::endl;
            
            // allocate an array to hold our command line arguments
            this->text_segment << "\t" << "mov rsi, 8" << std::endl // width of contained type is 8
                << "\t" << "mov rdi, r12" << std::endl  // contains 'r12' elements
                << "\t" << "pushfq" << std::endl
                << "\t" << "push rbp" << std::endl
                << "\t" << "mov rbp, rsp" << std::endl
                << "\t" << "call sinl_dynamic_array_alloc" << std::endl
                << "\t" << "mov rsp, rbp" << std::endl
                << "\t" << "pop rbp" << std::endl
                << "\t" << "popfq" << std::endl
                << "\t" << "push rax" << std::endl;

            // todo: get actual command-line arguments, convert them into SIN data types
            std::vector<std::shared_ptr<Expression>> cmd_args = {};
            for (
                auto it = main_symbol.get_formal_parameters().begin();
                it != main_symbol.get_formal_parameters().end(); 
                it++
            ) {
                // todo: get argument, create string, insert it into the array
            }

            // call the main function with SINCALL
            this->text_segment << this->sincall(main_symbol, cmd_args, 0).str();

            // preserve the return value and call SRE cleanup function
            this->text_segment << "\t" << "mov [rsp], rax" << std::endl;
            this->text_segment << "\t" << "mov rax, rsp" << std::endl
                << "\t" << "and rsp, -0x10" << std::endl
                << "\t" << "push rax" << std::endl
                << "\t" << "sub rsp, 8" << std::endl
                << "\t" << "call " << magic_numbers::SRE_CLEAN << std::endl
                << "\t" << "add rsp, 8" << std::endl
                << "\t" << "pop rsp" << std::endl;

            // restore main's return value and return
            this->text_segment << "\t" << "pop rax" << std::endl;
            this->text_segment << "\t" << "ret" << std::endl;
        }
        else if (main_function) {
            // if we found a symbol with the name 'main', but it wasn't a function, issue a warning
            compiler_warning(
                "Found a symbol 'main', but it is not a function",
                compiler_errors::MAIN_SIGNATURE,
                main_function->get_line_defined()
            );
        }

        // get the name for the generated assembly file
        // remove the extension from the file name and append ".s"
        size_t last_index = filename.find_last_of(".");
        if (last_index != std::string::npos)
            filename = filename.substr(0, last_index);
        filename += ".s";
        
        // now, save text, data, and bss segments to our outfile
        std::ofstream outfile;
        outfile.open(filename, std::ios::out);

        // first, write the text section
        outfile << "section .text" << std::endl;
        outfile << this->text_segment.str() << std::endl;

		// next, the .rodata
		outfile << "section .rodata" << std::endl;

        // we have bitmasks for single- and double-precision floats; they should be read-only
		outfile << "\t" << magic_numbers::SINGLE_PRECISION_MASK_LABEL << " dd 0x80000000" << std::endl;
		outfile << "\t" << magic_numbers::DOUBLE_PRECISION_MASK_LABEL << " dq 0x8000000000000000" << std::endl;	// todo: do we really need this? or is there an easier way to flip the sign?
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

compiler::compiler():
    evaluator(&this->structs)
{
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
}

compiler::~compiler() {
}
