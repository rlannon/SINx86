# SINx86

This repository contains an updated toolchain for SIN targeting x86-64 systems. This is an updated/refactored/improved version of the repository [`SIN-Language`](https://github.com/rlannon/SIN-Language), a compiler project that targeted a 16-bit VM. While this changes the compiler target, it also includes significant updates and improvements to elements that it retained from the original, particularly the lexer and parser. This is a part of the larger SIN Compiler Project which includes a [runtime environment](https://github.com/rlannon/SRE) and [standard library](https://github.com/rlannon/sinstdlib) in addition to this compiler.

SIN is a custom procedural programming language based on C, C++, and Python, designed as an exercise in compiler design. It was named in jest, after the Spanish word meaning "without" because if anyone were to ask you about the language, you would reply that it is without any real utility or purpose. Better languages exist, and this is purely an educational exercise in compiler development as well as C, C++, and assembly development in general. This repository contains a compiler for SIN with an x86 target; while I may add other architectures as I go on (such as ARM), x86 is the sole focus of the project for the time being.

## About the Language

### Background

SIN is very C-like; it is strictly procedural, utilizes strict type safety, doesn't allow implicit conversions, and gives the programmer a decent amount of freedom over the environment should they choose to use it. However, programmers shouldn't _have_ to worry about manual resource management, especially when running in an environment that allows us to have a runtime do it for us (i.e., inside a modern OS). As such, SIN utilizes a runtime environment for automatic memory management, includes reference types (like C++), and even contains certain safety features like runtime bounds checks on array and string indexing.

The central idea of SIN is to create a language with a fairly readable syntax that allows more safety than C but without all of the complicated object-oriented features of languages like Java. I wanted, essentially, a slightly more modernized version of C, one with some of the ease-of-development and safety features that are offered by modern languages. It aims to show that "modern" does not have to mean "object-oriented" (which I think Rust did well). That said, my goal isn't really to make a modern language competitor -- see the 'Goal of the Project' section for more.

### Sample

The following is an obligatory 'hello world!' sample, including the necessary boilerplate:

    include "stdio.sinh";
    def int main(alloc dynamic array<string> args) {
        @print("Hello, world!");
        return 0;
    }

Like Python, functions are defined with `def`, function calls look similar enough (though they are prefixed with `@` -- this is for a future language feature). Like C and Java, the entry point of entry program is 'main', though unlike Java (and like C), the return type and arguments for this function are not fixed. It's good practice to use a return type of `int` and a single argument `dynamic array<string> args`, but it's not strictly required (though the compiler will issue a warning).

For more information, check out the [guide](Basic%20Syntax.md).

## Goal of the Project

I cannot stress enough that this is a *learning exercise,* not an attempt at creating the next Python or Rust. This project is a hands-on way of learning compiler development. The purpose is not to create a particularly good compiler, but rather to serve as an exercise in compiler design and implementation. I intend on producing a *functioning* compiler, one that can be improved and expanded upon in the future. The goal is to generate code that *works,* and while I hope to improve the efficiency and overall funcionality of the compiler in the future, it is somewhat bodged together right now.

The fact that it's a learning exercise should explain why I decided to write a parser by hand, instead of using yacc/lex/bison/some other tool; doing so would not have allowed me to learn how parsers (can) work as deeply as I did by writing this one. It should also explain why I'm compiling directly from an abstract syntax tree into assembly, rather than using an intermediate representation like llvm, GNU RTL, or even C.

## Getting Started

### Installation / Build

There are currently no working binaries for the toolchain; you must compile and build it yourself (an admittedly bad makefile is included). Note that this program is written to generate code for Linux on an x86-64 machine, and so it there is no guarantee that this compiler will generate usable programs on other platforms (for the time being). However, the Linux-specific runtime code for the language is being moved from assembly to C code in the [runtime environment](https://github.com/rlannon/SRE), and so there shouldn't be any major compatibility issues with the code generated by this compiler (provided it is being run on an x64 machine). This code will be tested on Windows, Linux, and macOS on a few different x64 processors, so the toolchain will, hopefully, eventually run on all platforms. Because Linux and macOS both use the x64 System V ABI, there aren't any compatibility issues there, but a Windows target will likely need some modification. This will ultimately be the biggest hurdle for cross-compatibility.

### The SRE

In order to compile working SIN binaries, you will need a copy of the [SIN Runtime Environment](https://github.com/rlannon/SRE), a small library which provides necessary runtime support for the language. It must be statically-linked to all SIN programs in order for them to produce a working executable. Although the SRE is currently unfinished, it implements the necessary functionality for the langauge features which are currently supported by this code generator.

### The SIN Standard Library

SIN, like many other programming languages, has a standard library that includes various features that flesh out the language and integrate it with the host environment. Like the SRE, this library is located in a [separate repository](https://github.com/rlannon/sinstdlib). While it is not required to produce working binaries, it will ultimately serve to make the language more useful (as I/O with the environment is not included by default). As such, it is highly encouraged that any brave souls who decide to try out this programming language build and use the standard library.

Note that the aforementioned repo should be fairly portable, but as is the case with the the rest of the compiler project, I can't make any guarantees (for the time being) as the current focus is Linux.

### Third-Party Software

This is not a full compiler suite, and as such, a few third-party applications are required. Without them, the compiler will be not be able to generate working binaries for the programs you write. The required programs are:

* GCC, G++ - The SIN compiler relies on certain C and C++ functionality for its runtime environment, and without a working C++ compiler, linking the standard library with an assembled SIN program will be impossible (unless a custom SRE implementation that does not use either language is used, in which case _some_ linker will still be required). C++14 is used in this compiler.
* NASM - The compiler does not generate object code directly; instead, it serializes it into x64 assembly in Intel syntax. I use [nasm](https://nasm.us) as my assembler, so all of the macros I use in the generated code are targeted for that assembler.

It is also important to note that because SIN programs utilize standard library functions that define `_start`, SIN programs begin execution from `_main`, which is invoked by `_start`. However, SIN programs would *normally* utilize `_start` to initialize the runtime, command-line arguments, etc., this functionality must be moved into `_main`. While a compiler could move these routines to the start of the user's `_main` implementation, this compiler utilizes decoration to call `SIN_main` from `_main`. By slightly decorating every function and variable name, we can work around this issue pretty easily, but this is by no means the only (or even best) way of solving the problem.

### Using SIN

I intend on using [GitHub Pages](rlannon.github.com/SINx86) for introductory programming materials. Note that currently, not everything is up-to-date, and the site will be updated periodically.

To build projects, the following steps should be followed:

* Ensure `make` is installed
* Build the SRE using the provided makefile
* Build the compiler using the provided makefile
* Build all requisite `.sin` files for your project using `sinx86`
* Assemble them using `nasm` -- for ecxample, `nasm -f <format> <path to assembly file>`, optionally specifying the `.o` files with the `-o` option and allowing debug symbols with the `-g` option. Since all object files ultimately get linked by GCC, you could probably use ELF (`-f elf64`) on any system for the intermediate object files because they're not executable by themselves.
* Link with G++ using `g++ <list object files> -L <path to SRE> -l SRE`, again optionally specifying the outfile with `-o`

Note there is no reason you can't use `make` for SIN projects. Doing so would probably make life a whole lot easier!

#### Language Samples and Benchmarks

Included with this project are a folder of various sample SIN files to test the compiler's functionality. They can also serve as general syntax/usage references.

Within the samples folder is a folder called `benchmarks`, which includes various algorithms in SIN, Python, and C to test compile and execution times and serve as benchmark tests.

## Future Goals

I hope to use this project as a stepping stone to develop other languages and explore other features, such as compilers for object-oriented programming languages. For this project specifically, I hope to add in:

* Various compiler optimizations
* Debugging support
* VSCode integration (there's currently a [syntax highlighting extension!](https://github.com/rlannon/vscode-sin-lang))
* Native support for UTF-8 instead of ASCII

In the doc files you will also find a number of documents mentioning things that are not currently supported by the compiler but will be in the future (e.g., anonymous functions and tuples). I also intend on writing a superset of SIN in the future to include more language features and enable first-class concurrency.
