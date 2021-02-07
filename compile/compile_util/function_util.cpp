/*

function_util.cpp
Copyright 2020 Riley Lannon

*/

#include "function_util.h"
#include "../../parser/Expression.h"
#include "../../parser/Statement.h"
#include "symbol_table.h"
#include "magic_numbers.h"

#include "utilities.h"

std::string function_util::call_sincall_subroutine(std::string name) {
    // Sets up a stack frame, calls a function, restores frame
    
    std::stringstream call_ss;

    call_ss << "\t" << "pushfq" << std::endl;
    call_ss << "\t" << "push rbp" << std::endl;
    call_ss << "\t" << "mov rbp, rsp" << std::endl;
    call_ss << "\t" << "call " << name << std::endl;
    call_ss << "\t" << "mov rsp, rbp" << std::endl;
    call_ss << "\t" << "pop rbp" << std::endl;
    call_ss << "\t" << "popfq" << std::endl;

    return call_ss.str();
}

template function_symbol function_util::create_function_symbol(const FunctionDefinition&, bool, bool, const std::string&, unsigned int, bool);
template function_symbol function_util::create_function_symbol(const Declaration&, bool, bool, const std::string&, unsigned int, bool);
template <typename T>
function_symbol function_util::create_function_symbol(
    const T& def,
    bool mangle,
    bool defined,
    const std::string& scope_name,
    unsigned int scope_level,
    bool is_method
) {
    /*

    create_function_symbol
    Creates a symbol for a function based on either a definition or a declaration

    This function is responsible for turning the Statement objects containing parameters into symbol objects, but it _does not_ add them to the symbol table (as it is not a member of compiler)

    @param  def The definition or declaration from which to create our symbol
    @return A symbol containing the function signature

    */

    // todo: this function marks the scope name of parameters without mangling
    std::string name = mangle ? symbol_table::get_mangled_name(def.get_name(), scope_name) : def.get_name();
    std::string inner_scope_name = name;
    unsigned int inner_scope_level = scope_level + 1;
    size_t stack_offset = 0;

    // construct our formal parameters
    std::vector<symbol> formal_parameters;

    // if we have a nonstatic method, we need to make sure the first parameter is 'ref<T> this' (unless it was provided -- in which case, validate it)
    bool has_this_parameter = false;
    symbol this_parameter(
        "this",
        inner_scope_name,
        inner_scope_level,
        DataType(
            REFERENCE,  // Default type for 'this' is ref< T >
            DataType(
                STRUCT,
                DataType(),
                symbol_qualities(),
                nullptr,
                scope_name
            ),
            symbol_qualities()
        ),
        0,
        true,
        def.get_line_number()
    );
    this_parameter.set_as_parameter();
    this_parameter.set_initialized();

    if (
        is_method && 
        !def.get_type_information().get_qualities().is_static()
        && def.get_formal_parameters().empty()
    ) {
        formal_parameters.push_back(this_parameter);
        has_this_parameter = true;
    }

    // now, determine which registers can hold which parameters
    for (size_t i = 0; i < def.get_formal_parameters().size(); i++) {
        // get the parameter
        auto param = def.get_formal_parameters().at(i);

        // create the symbol based on our statement
        symbol param_sym;

        // cast to the appropriate symbol type
        if (param->get_statement_type() == DECLARATION) {
            auto param_decl = static_cast<const Declaration*>(param);
            param_sym = generate_symbol(
                *param_decl,
                param_decl->get_type_information().get_width(),
                inner_scope_name,
                inner_scope_level,
                stack_offset
            );
        } else if (param->get_statement_type() == ALLOCATION) {
            auto param_alloc = static_cast<const Allocation*>(param);
            DataType t = param_alloc->get_type_information();
            param_sym = generate_symbol(
                *param_alloc,
                t.get_width(),
                inner_scope_name,
                inner_scope_level,
                stack_offset
            );
        } else {
            // todo: remove? these errors should be caught by the parser
            throw CompilerException("Invalid statement type in function signature", compiler_errors::ILLEGAL_OPERATION_ERROR, def.get_line_number());
        }

        // ensure the first parameter is 'this' if we need it
        if (i == 0 && is_method && !has_this_parameter) {
            // ensure we have a typename of 'this', make sure it's the right type
            if (param_sym.get_name() == "this") {
                auto t = param_sym.get_data_type();
                if (
                    (
                        t.get_primary() == REFERENCE 
                        || t.get_primary() == PTR
                    ) 
                    && t.get_subtype() == STRUCT 
                    && t.get_struct_name() == scope_name
                ) {
                    has_this_parameter = true;
                }
                else {
                    throw CompilerException(
                        "Expected 'this' parameter to have type of ptr< " + scope_name + " > or ref< " + scope_name + " >",
                        compiler_errors::INCORRECT_THIS_TYPE,
                        def.get_line_number()
                    );
                }
            }
            else if (!def.get_type_information().get_qualities().is_static()) {
                // we need to add a 'this' parameter if it's a nonstatic method
                formal_parameters.push_back(this_parameter);
                has_this_parameter = true;
            }
        }

        // make sure it's marked as a paramter and marked as initialized (so that we don't get errors about uninitialized data in the function)
        param_sym.set_as_parameter();
        param_sym.set_initialized();
        formal_parameters.push_back(param_sym);
    }

    // now, if we have a 'this' parameter and it's a static method, it's an error
    if (has_this_parameter && def.get_type_information().get_qualities().is_static()) {           
        throw CompilerException(
            "Cannot have 'this' parameter for static member functions",
            compiler_errors::ILLEGAL_THIS_PARAMETER,
            def.get_line_number()
        );
    }

    // construct the object
    function_symbol to_return(
        name,
        def.get_type_information(),
        formal_parameters,
        scope_name,
        scope_level,
        def.get_calling_convention(),
        defined,
        def.get_line_number()
    );

    // finally, return the function symbol
    return to_return;
}


std::stringstream function_util::call_sre_free(symbol& s) {
    /*

    call_sre_free
    Calls _sre_free on the specified symbol

    */

    return call_sre_mam_util(s, magic_numbers::SRE_FREE);
}

std::stringstream function_util::call_sre_add_ref(symbol& s) {
    /*

    call_sre_add_ref
    Calls the function to add a reference for the given symbol

    */

    return call_sre_mam_util(s, magic_numbers::SRE_ADD_REF);
}

std::stringstream function_util::call_sre_mam_util(symbol& s, std::string func_name) {
    /*

    call_sre_mam_util
    A utility that actually generates code

    Takes a symbol and the function name and generates code for it
    These functions may be:
        * _sre_add_ref
        * _sre_free

    */

    std::stringstream gen;
    std::stringstream get_addr;

    if (s.get_data_type().get_qualities().is_static()) {
        get_addr << "\t" << "lea rdi, " << s.get_name() << std::endl;
    }
    else if (
        (s.get_data_type().get_primary() == PTR && s.get_data_type().get_qualities().is_managed()) ||
        s.get_data_type().is_reference_type()
    ) {
        get_addr << "\t" << "mov rdi, [rbp - " << s.get_offset() << "]" << std::endl;
    }
    else {
         // if we have a negative number for the offset, add it instead
        if (s.get_offset() < 0) {
            get_addr << "\t" << "lea rbx, [rbp + " << -s.get_offset() << "]" << std::endl;
        }
        else {
            get_addr << "\t" << "lea rbx, [rbp - " << s.get_offset() << "]" << std::endl;
        }
        get_addr << "\t" << "mov rdi, [rbx]" << std::endl;
    }

    gen << get_addr.str();
    gen << "\t" << "pushfq" << std::endl;
    gen << call_sre_function(func_name);
    gen << "\t" << "popfq" << std::endl;

    return gen;
}

std::string function_util::call_sre_function(std::string func_name) {
    // Calls an SRE function
    std::stringstream call_ss;
    call_ss << "\t" << "mov rax, rsp" << std::endl	// ensure we have 16-byte stack alignment
        << "\t" << "and rsp, -0x10" << std::endl
        << "\t" << "push rax" << std::endl
        << "\t" << "sub rsp, 8" << std::endl;
    call_ss << "\t" << "call " << func_name << std::endl;
    call_ss << "\t" << "add rsp, 8" << std::endl
        << "\t" << "pop rsp" << std::endl;
    
    return call_ss.str();
}
