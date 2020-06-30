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

and we want to use them in our main file. We can simply use the `include` file to include these functions and allow `main.sin` to use them:

    include "simple_math.sin"

    def int main() {
        alloc int x: 10;
        alloc int y: 30;
        return multiply(10, 30);
    }

In this case, `multiply` will be in the symbol table already as it will have been processed by the `include` statement up top. Include statements may be placed anywhere, but only the code that comes after them will actually be able to use included symbols. Unlike C and C++, duplicate includes are automatically ignored (removing the need for something like `#pragma once` or other preprocessor include guards).

### How they work

When an `include` statement is encountered, the compiler takes a few steps:

* Tokenizes and produces an abstract syntax tree (AST) for the included file
* Walks through the AST and looks for allocations, definitions, and declarations, adding their symbol information to the source file's symbol, struct, and constant tables, along with the following:
  * If a function definition is found, it will add `extern <function name>` to the source file
  * If a struct definition is found, it will generate no assembly, but the struct's information will be added to the compiler's struct table
  * If a declaration is found, it will generate the appropriate information for the declaration and add it to the appropriate table
  * If an allocation is found, it will generate the symbol and add it to the table. However, it will not actually perform any allocation. It will then add `extern <symbol name>` to the source file

The code for the included file is *not* generated when included; rather, it must be compiled separately and linked. So, in the above example, we would produce the executable by doing something like:

    # generate an object file for simple_math
    sinx86 simple_math.sin
    nasm -f elf64 -o simple_math.o simple_math.s

    # build the main file
    sinx86 main.sin
    nasm -f elf64 -o main.o main.s
    
    # link the project
    g++ main.o simple_math.o -L <path to SRE> -l SRE

When the main file is built, and the `include` is encountered, it will see the function definitions and add them to the 
