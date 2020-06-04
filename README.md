# SINx86

This repository contains an updated toolchain for SIN targeting x86-64 systems. This is an updated/refactored/improved version of the repository [`SIN-Language`](http://github.com/rlannon/SIN-Language), a compiler project that targeted a 16-bit VM. While this changes the compiler target, it also includes significant updates and improvements to elements that it retained from the original, particularly the lexer and parser.

SIN is a custom procedural programming language based on C, C++, and Python, designed as an exercise in compiler design. It is named after the Spanish word meaning "without" because if anyone were to ask you about the language, you would reply that it is without any real utility or purpose. This repository contains a compiler for SIN with an x86 target; while I may add other architectures as I go on, x86 is the sole focus of the project for the time being.

## About the Language

### Background

SIN is very C-like; it is strictly procedural, utilizes strict type safety, doesn't allow implicit conversions, and gives the programmer a decent amount of freedom over the environment should they choose to use it. However, programmers shouldn't *have* to worry about manually allocating and freeing memory, especially when we have the resources to let a runtime environment do it automatically. As such, SIN utilizes a runtime environment for automatic memory management, it includes reference types (like C++), and even contains runtime bounds-checking on arrays.

The central idea of SIN is to create a language with a fairly readable syntax that allows more safety than C but without all of the complicated object-oriented features of languages. I wanted, essentially, a slightly more modernized version of C, one with some of the ease-of-development and safety features that are offered by modern languages. It aims to prove that "modern" does not have to mean "object-oriented" (which I think Rust did well). That said, my goal isn't really to make a modern language competitor -- see the 'Goal of the Project' section for more.

### Sample

The following is an obligatory 'hello world!' sample:

    def int main(alloc dynamic array<string> args) {
        @print("Hello, world!");
        return 0;
    }

Like Python, functions are defined with `def`, function calls look similar enought (though they are prefixed with `@` -- this is for a future language feature). Like C and Java, the entry point of entry program is 'main', though unlike Java (and like C), the return type and arguments for this function are not fixed. It's good practice to use a return type of `int` and the argument `dynamic array<string> args`, but it's not strictly required.

For more information, check out the [guide](Basic%20Syntax.md).

## Goal of the Project

I cannot stress enough that this is a *learning exercise,* not an attempt at creating the next Python or Rust. This project is a hands-on way of learning compiler development. The purpose is not to create a particularly good compiler, but rather to serve as an exercise in compiler design and implementation. I intend on producing a *functioning* compiler, one that can be improved and expanded upon in the future. The goal is to generate code that *works,* and while I hope to improve the efficiency and overall funcionality of the compiler in the future, it is somewhat bodged together right now.

The fact that it's a learning exercise should explain why I decided to write a parser by hand, instead of using yacc/lex/bison/some other tool; doing so would not have allowed me to learn how parsers (can) work as deeply as I did by writing this one. It should also explain why I'm compiling directly from an abstract syntax tree into assembly, rather than using an intermediate representation like lldb.

In other words: this compiler isn't *supposed* to be any good. The language spec might be alright, but if you try to use this to actually write any software, you've been warned.

## Getting Started

### Installation / Build

There are currently no working binaries for the toolchain; you must compile and build it yourself (an admittedly bad makefile is included). Note that this program is written to generate code for Linux on an x86-64 machine, and so it there is no guarantee that this compiler will generate usable programs on other platforms (for the time being). However, the Linux-specific runtime code for the language (interaction with an ABI) is being moved to the [runtime environment](https://github.com/rlannon/SRE), and so there shouldn't be any major compatibility issues with the code generated by this compiler, provided the code is being run on an x64 machine. It is the SRE that will prove to be the larger hurdle. However, adapting the SRE to a new x64 environment will only involve modifying a few assembly routines, and so this will not be a permanent problem.

### The SRE

In order to compile working SIN binaries, you will need a copy of the [SIN Runtime Environment](https://github.com/rlannon/SRE), a small library which provides necessary runtime support for the language. Currently, the library is unfinished and so any compiled SIN programs will likely fail at the link stage. Given the compiler is also unfinished, this shouldn't prove to be much of an issue.

### The SIN Standard Library

SIN, like many other programming languages, has a standard library that includes various features that flesh out the language and integrate it with the host environment. This library is located in a [separate repository](https://github.com/rlannon/sinstdlib). While it is not required, it will ultimately serve to make the language more useful (as I/O with the environment is not included by default), and so it is highly encouraged that any brave souls who decide to try out this programming language build and use it.

Note that the aforementioned repo should be fairly portable, as it is implemented in a combo of SIN and C, but I can't make any guarantees (for the time being) as the current focus is Linux.

### Third-Party Software

This is not a full compiler suite, and as such, a few third-party applications are required. Without them, the compiler will be not be able to generate working binaries for the programs you write. The required programs are:

* Working C and C++ compilers (such as GCC/G++) - the SIN compiler relies on certain C and C++ functionality for its runtime environment, and without a working C++ compiler, linking the standard library with an assembled SIN program will be impossible (unless a custom SRE implementation that does not use either language is used, in which case *some* linker will be required)
* An assembler (NASM is recommended) - the compiler does not generate object code directly; instead, it generates x64 assembly in Intel syntax, meaning an assembler is required. While I prefer NASM, any assembler that supports Intel x86 syntax should work without too much difficulty

Note that SIN programs utilize the C standard library (and certain functionality from C++) that define `_start`. As such, SIN programs are started from `main` (SIN functions use minor name mangling, unless specified as `extern`), which is called by `_start`. A different implementation could do this differently, but this is the easiest workaround for the time being.

### Using SIN

I intend on using [GitHub Pages](rlannon.github.com/SINx86) for introductory programming materials. Note that currently, not everything is up-to-date, and the site will be updated periodically.

## Future Goals

I hope to use this project as a stepping stone to develop other languages and explore other features, such as compilers for object-oriented programming languages. For this project specifically, I hope to add in:

* Various compiler optimizations
* Debugging support
* VSCode integration (there's currently a [syntax highlighting extension!](https://github.com/rlannon/vscode-sin-lang))
* Native support for UTF-8 instead of ASCII
