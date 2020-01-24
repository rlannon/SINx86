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

    @param  to_add  The symbol we want to add
    @param  line    The line number where the allocation occurs

    @throws DuplicateSymbolException

    */

    bool ok = this->symbol_table.insert(
        std::make_pair(
            to_add.get_name(),
            std::make_shared<T>(to_add)
        )
    ).second;

    // throw an exception if the symbol could not be inserted
    if (!ok) {
        throw DuplicateSymbolException(line);
    }
}

struct_info compiler::get_struct_info(std::string struct_name, unsigned int line) {
    /*
    
    get_struct_info
    Looks up a struct with the given name in the struct table

    @param  struct_name The name of the struct to find
    @param  line    The line where the lookup is needed
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
            // Included files will not be added more than once in any compilation process -- so we don't need anything like "pragma once"; this is accomplished through the use of std::set
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
            // return statements may only occur within functions; if we have a nullptr, then we aren't inside a function
            if (signature) {
                ReturnStatement *return_stmt = dynamic_cast<ReturnStatement*>(s.get());
                compile_ss << this->handle_return(*return_stmt, *(signature.get())).str() << std::endl;
            } else {
                throw IllegalReturnException(s->get_line_number());
            }
            break;
        }
        case IF_THEN_ELSE:
            // todo: ITE
            break;
        case WHILE_LOOP:
            // todo: while
            break;
        case FUNCTION_DEFINITION:
        {
            FunctionDefinition *def_stmt = dynamic_cast<FunctionDefinition*>(s.get());
            compile_ss << this->define_function(*def_stmt).str() << std::endl;
            break;
        }
        case STRUCT_DEFINITION:
            // todo: add struct to the struct table so we can utilize them
            break;
        case CALL:
            // todo: call a function
            // todo: solidify SIN calling convention
            break;
        case INLINE_ASM:
            // todo: write ASM to file
            break;
        case FREE_MEMORY:
            /*

            'free' may be used with either automatic or dynamic memory -- it may not be used with const or static

            however, it has very little effect on automatic memory; it just marks the memory as freed, preventing any future writes to it

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
    // initialize our number trackers to 0
    this->strc_num = 0;
    this->scope_block_num = 0;
    this->max_offset = 0;
    
    // initialize the scope
    this->current_scope_name = "global";
    this->current_scope_level = 0;
}

compiler::~compiler() {
    // todo: destructor
}
