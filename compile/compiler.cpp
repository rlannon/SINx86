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

    // use the unordered_map::find function
    std::unordered_map<std::string, std::shared_ptr<symbol>>::iterator it = this->symbol_table.find(name);

    // if the symbol wasn't found, throw an exception
    if (it == this->symbol_table.end()) {
        throw SymbolNotFoundException(line);
    } else {
        return it->second;
    }
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
	size_t pos = to_add.get_name().find("sinl_");
	if (pos != std::string::npos && pos == 0) {
		compiler_warning("'sinl_' is a reserved prefix for SIN runtime environment symbols. Using this prefix may result in link-time errors due to multiple symbol definition.");
	}

	// insert the symbol
    bool ok = this->symbol_table.insert(
        std::make_pair(
            to_add.get_name(),
            std::make_shared<T>(to_add)
        )
    ).second;

    // throw an exception if the symbol could not be inserted
    if (!ok) {
		// if it's a function we are adding, throw a duplicate *definition* exception; else, it's a duplicate symbol
		if (to_add.get_symbol_type == SymbolType::FUNCTION_SYMBOL)
			throw DuplicateDefinitionException(line);
		else
			throw DuplicateSymbolException(line);
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

	size_t pos = to_add.get_struct_name().find("sinl_");
	if (pos != std::string::npos && pos == 0) {
		compiler_warning("'sinl_' is a reserved prefix for SIN runtime environment symbols. Using this prefix may result in link-time errors due to multiple symbol definition.");
	}

	bool ok = this->struct_table.insert(
		std::make_pair<>(
			to_add.get_struct_name(),
			to_add
		)
	).second;

	if (!ok) {
		throw DuplicateDefinitionException(line);
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

    // look up our struct
    std::unordered_map<std::string, struct_info>::iterator it = this->struct_table.find(struct_name);

    // if found, return its data; else, throw an UndefinedException
    if (it == this->struct_table.end()) {
        throw UndefinedException(line);
    } else {
        return it->second;
    }
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
            // Included files will not be added more than once in any compilation process -- so we don't need anything like "pragma once"; this is to be accomplished through the use of std::set
            break;
        case DECLARATION:
        {
            // handle a declaration
            Declaration *decl_stmt = dynamic_cast<Declaration*>(s.get());

            // we need to ensure that the current scope is global -- declarations can only happen in the global scope, as they must be static
            if (this->current_scope_name == "global" && this->current_scope_level == 0) {
                this->handle_declaration(*decl_stmt);
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
            compile_ss << this->assign(*assign_stmt).str() << std::endl;
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
			
			// then we need to evaluate the expression; if the final result is 'true', we continue in the tree; else, we branch to 'else'
			// if there is no else statement, it falls through to 'done'
			compile_ss << this->evaluate_expression(ite->get_condition(), ite->get_line_number()).str() << std::endl;
			compile_ss << "\t" << "jz sinl_ite_else_" << current_scope_num << std::endl;	// compare the result of RAX with 0; if true, then the condition was false, and we should jump
			
			// compile the branch
			compile_ss << this->compile_ast(*ite->get_if_branch().get()).str();

			// now, we need to jump to "done" to ensure the "else" branch is not automatically executed
			compile_ss << "\t" << "jmp sinl_ite_done_" << current_scope_num << std::endl;
			compile_ss << "sinl_ite_else_" << current_scope_num << ":" << std::endl;

			// compile the branch, if one exists
			if (ite->get_else_branch().get()) {
				compile_ss << this->compile_ast(*ite->get_else_branch().get()).str();
			}

			// clean-up
			compile_ss << "sinl_ite_done_" << current_scope_num << ":" << std::endl;
			this->scope_block_num += 1;
			break;
		}
		case WHILE_LOOP:
            // todo: while
            break;
        case FUNCTION_DEFINITION:
        {
            FunctionDefinition *def_stmt = dynamic_cast<FunctionDefinition*>(s.get());

			// ensure the function has a return value in all control paths
			if (returns(*def_stmt->get_procedure().get())) {
				compile_ss << this->define_function(*def_stmt).str() << std::endl;
			} else {
				throw NoReturnException(s->get_line_number());
			}
            break;
        }
        case STRUCT_DEFINITION:
		{
			StructDefinition *def_stmt = dynamic_cast<StructDefinition*>(s.get());

			struct_info defined = define_struct(*def_stmt);
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
            // todo: write ASM to file
            break;
        case FREE_MEMORY:
            /*

            'free' may be used with either automatic or dynamic memory -- it may not be used with const or static
            however, it has very little effect on automatic memory; it just marks the memory as freed, preventing any future writes to it

			'free' is also safe in that it will not trigger a fault if you call 'free' on the same data twice (though if the compiler sees this may happen, then it will generate a warning)

            */

            break;
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

	// todo: is there a more efficient way to handle this? maybe in blocked scopes we save a *copy* of the map? that might involve *huge* memory overhead (depending on the size of the user's program) that would be avoided with this method, though...

	// delete all symbols from the current scope upon exiting -- both from this symbol table AND from the constant evaluation table
	std::unordered_map<std::string, std::shared_ptr<symbol>>::iterator it = this->symbol_table.begin();
	while (it != this->symbol_table.end()) {
		if (it->second->get_scope_name() == this->current_scope_name && it->second->get_scope_level() == this->current_scope_level) {
			it = this->symbol_table.erase(it);
		}
		else {
			it++;
		}
	}
	// todo: call compile_time_evaluator::remove_symbols_in_scope to remove symbols
	// note the scope name and level are updated elsewhere

    return compile_ss;
}

void compiler::generate_asm(std::string filename, Parser &p) {
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
        // create our abstract syntax tree
        StatementBlock ast = p.create_ast();

        // The code we are generating will go in the text segment -- writes to the data and bss sections will be done as needed in other functions
        this->text_segment << this->compile_ast(ast).str();

        // now, we want to see if we have a function 'main' in the symbol table
        try {
            // if we have a main function in this file, then insert our entry point (set up stack frame and call main)
            std::shared_ptr<symbol> main_function = this->lookup("main", 0);

            // insert our wrapper for the program
            this->text_segment << "global _start" << std::endl;
            this->text_segment << "_start:" << std::endl;

            // set up the stack frame
            this->text_segment << "\t" << "push rbp" << std::endl;
            this->text_segment << "\t" << "mov rbp, rsp" << std::endl;

            // todo: set up program arguments for main?

            // call main
            this->text_segment << "\t" << "call main" << std::endl;

            // restore old stack frame
            this->text_segment << "\t" << "mov rsp, rbp" << std::endl;
            this->text_segment << "\t" << "pop rbp" << std::endl;

            // exit the program using the linux syscall
            this->text_segment << "\t" << "mov rbx, rax" << std::endl;
            this->text_segment << "\t" << "mov rax, 0x01" << std::endl;
            this->text_segment << "\t" << "int 0x80" << std::endl;
        } catch (SymbolNotFoundException &e) {
            // print a warning saying no entry point was found -- but SIN files do not have to have entry points, as they might be included
            compiler_warning("Note: no entry point found in file", 0);
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

        // close the outfile
        outfile.close();
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
    this->scope_block_num = 0;
    this->max_offset = 8;   // should be 8 (a qword) because of the way the x86 stack works
    
    // initialize the scope
    this->current_scope_name = "global";
    this->current_scope_level = 0;
}

compiler::~compiler() {
    // todo: destructor
}
