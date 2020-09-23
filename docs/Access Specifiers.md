# SIN Documentation

## Visibility and Access Specifiers

In SIN, the scope of any given piece of data is important. While it is possible to have static variables that are allocated at compile time and live outside of any function or struct, this does not necessarily make them _global._ Instead, they are considered _static_ and limited to the scope of that particular file. Note that this doesn't just apply to variables and constants, it applies to _all symbols,_ meaning functions as well.

As an example, take the following case:

    // sample.sin
    def int mult(alloc int a, alloc int b) {
        return a * b;
    }

    // main.sin
    include "sample.sin";

    def int main(alloc dynamic array<string> args) {
        alloc int n: @mult(10, 20); // Error; 'mult' is not defined
        return n + 1;
    }

In this example, although the file containing the function `mult` was included, the function was not marked as globally available, and is therefore limited in scope to the file `sample.sin`. To make a piece of data truly global -- available to the entire program -- there are two ways this can be accomplished. One makes use of the `extern` keyword, while the other utilizes [included files](Includes), though they are slightly different.

### The `extern` keyword

The `extern` keyword does two things:

* Prevents name decoration for the symbol (like in C++)
* Makes the symbol available globally

This is the ideal solution if some data from a SIN program is to be used by another program (e.g., a SIN function is to be called from a C program) because it eliminates any name decoration that the compiler might use. However, in this scenario, it is far from ideal. While we could make the program work by saying

    // sample.sin
    def extern int mult(alloc int a, alloc int b) {
        return a * b;
    }

it is much better to use Declarative SIN and an `include` to make the symbol global.

### Declarative SIN

The other way to make a symbol global is to utilize Declarative SIN and separate declaration from definition. For example:

    // sample.sinh
    // the declaration
    decl int mult(decl int a, decl int b);  // declares the function, making it globally available

    // sample.sin
    // the definition
    
    // include the header so the compiler knows this is the definition for that symbol; else, this implementation will not be globally available
    include "sample.sinh";
    
    def int mult(alloc int a, alloc int b) {
        return a * b;
    }

    // main.sin
    include "sample.sinh";

    def int main(alloc dynamic array<string> args) {
        alloc int n: @mult(10, 20); // OK; mult was declared in sample.sinh
        return n + 1;
    }

Here, we use a style of SIN programming called _Declarative SIN._ The idea behind Declarative SIN is that it doesn't generate any code that will go into the `.text` segment, and only create either an `extern <symbol name>` assembler directive or add an entry into the struct table. This has the benefit of faster compilation time, as everything that is parsed in the included file will be used.

For more information, see the [includes](Includes).

### But why?

The reason SIN works this way is for four key reasons:

* it allows the assembler to easily recognize which symbols need to be marked as `global` and `extern`;
* it allows us to save on compile time by ignoring symbols that aren't explicitly marked for global access, especially when Declarative SIN is used;
* it prevents the use of global variables except when explicitly called for in order to discourage their use;
* it encourages the separation of declaration and definition, allowing for more modularity in compilation
