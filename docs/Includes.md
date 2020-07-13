# SIN Documentation

## Included Files

SIN, like many other languages, allows files to be included for both modularity and organizational purposes. The system used in SIN is not a proper module system, but rather is like the C-style system with the preprocesor directive `#include`. However, in SIN, the keyword `include` is used.

### How to use them

Let's say we have a file `simple_math.sin` that defines some functions:

    def int multiply(alloc int a, alloc int b) {
        return a * b;
    }

    def int divide(alloc int a, alloc int b) {
        return a / b;
    }

and we want to use them in our main file. We can simply use the `include` file to include these functions and allow `main.sin` to use them (though this code would not compile -- this will be dicussed later):

    include "simple_math.sin";

    def int main() {
        alloc int x: 10;
        alloc int y: 30;
        return @multiply(10, 30);
    }

In this case, `multiply` will be in the symbol table already as it will have been processed by the `include` statement up top. Include statements may be placed anywhere, but only the code that comes after them will actually be able to use included symbols. Unlike C and C++, duplicate includes are automatically ignored (removing the need for something like `#pragma once` or other preprocessor include guards).

### How they work

When an `include` statement is encountered, the compiler takes a few steps:

* Tokenizes and produces an abstract syntax tree (AST) for the included file
* Walks through the AST and looks for allocations, definitions, and declarations, adding their symbol information to the source file's symbol, struct, and constant tables, along with the following:
  * If a function definition is found, one of the following will happen:
    * If the function is declared as `extern`, it will create the symbol and add it to the table
    * Else, if the symbol does not already exist from a `decl` statement, an error will be generated
    * If the symbol already exists from a `decl` statement, it will be ignored -- the compiler already has the requisite symbol information
  * If a struct definition is found, it will generate no assembly, but the struct's information will be added to the compiler's struct table
  * If an allocation is found, it will do one of the following:
    * If the symbol is declared as `extern`, it will generate the symbol and add it to the table. However, it will not actually perform any allocation
  * If a declaration is found, it will generate the appropriate information for the declaration and add it to the appropriate table, marking the symbol as undefined so that the corresponding `def` or `alloc` does not cause any issues

The code for the included file is *not* generated when included; rather, it must be compiled separately and linked. So, in the above example, we would produce the executable by doing something like:

    # generate an object file for simple_math
    sinx86 simple_math.sin
    nasm -f elf64 -o simple_math.o simple_math.s

    # build the main file
    sinx86 main.sin
    nasm -f elf64 -o main.o main.s
    
    # link the project
    g++ main.o simple_math.o -L <path to SRE> -l SRE

When the main file is built, and the `include` is encountered, it will see the function definitions and add them to the symbol table, but it won't actually look at any code within the function. While this is not very efficient (as the whole file will be parsed for an include, but must be compiled separately), it prevents certain issues that would arise from compiling everything at once. For example, this allows portions of the project to be recompiled without recompiling all of its dependencies. Efficiency issues are also solved through the requirement of Declarative SIN.

#### Declarative SIN

To avoid efficiency issues that may arise when including files, the compiler requires that header files utilize a style of SIN known as *Declarative SIN,* typically using the extension `.sinh` (though this is not required -- akin to the `.h` and `.c` distinction in C, where the files are just text files and have no real differences to the compiler). Declarative SIN allows only the following:

* `decl` statements to declare the existence of a function, data, or struct
* Struct definitions

This is akin to C and C++ header files, where the `.h` file contains declarations and the `.c` or `.cpp` file contains the actual implementation.

SIN `decl` statements will ensure that the assembler directive `extern` is used, indicating a global symbol. This is different from the SIN `extern` keyword, which will generate both the `extern` keyword *and* prevent name mangling. This means that for the above example, the proper way to do this would be either the following:

    // simple_math.sin
    def extern int multiply(alloc int a, alloc int b) {
        return a * b;
    }

    def extern int divide(alloc int a, alloc int b) {
        return a / b;
    }

    ...

    // main.sin
    include "simple_math.sin";

    def int main() {
        alloc int x: 10;
        alloc int y: 30;
        return @multiply(x, y);
    }

This is fine, but using Declarative SIN, the following would be done:

    // simple_math.sinh
    decl int multiply(decl int a, decl int b);
    decl int divide(decl int a, decl int b);

    ...

    // simple_math.sin
    include "simple_math.sinh";  // includes the declarations for global symbols

    // the compiler will ensure this signature matches the one in the declaration
    def int multiply(alloc int a, alloc int b) {
        return a * b;
    }

    def int divide(alloc int a, alloc int b) {
        return a / b;
    }

    ...

    // main.sin
    include "simple_math.sinh";

    def int main() {
        alloc int x: 10;
        alloc int y: 30;
        return @multiply(x, y);
    }

This allows `simple_math.sin` to be compiled separately from `main.sin`, meaning any changes to it will be ignored when `main.sin` is compiled and linked to the final executable. The compiler will be made aware of its functions through the `.sinh` file. This also has the benefit of giving us a lot less code to parse. Note that if Declarative SIN is not used, trying to compile the first example in this file would give an error upon reaching the function signature in the included file, stating:

    Attempt to include a non-globalized symbol in SIN file; use Declarative SIN or "extern"

This is because by default, no functions or data in SIN will be visible outside the source file. If a `decl` statement is used, this means that there exists some global symbol that will be linked with the file; these must be defined somewhere, and that definition must include the declaration so the compiler is aware of the association. This is why the `extern` keyword may be used instead, though it is not recommended.

However, struct definitions are *always* considered external; they do not create a symbol, but rather a data structure that can be used elsewhere. It is precisely because they are not symbols that they do not require the `extern` keyword or a declaration for use in other files, and therefore, `def struct` may be -- and actually *should* be -- used in Declarative SIN. The use of `decl struct` should only be used when a circular dependence is present. Note that the use of `decl struct` means that no struct member information will be available, and so only pointers may be used when some struct's existence is known only from a declaration.
