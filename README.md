# SINx86

This repository contains an updated toolchain for SIN targeting x86-64 Linux systems. This is an updated/refactored/improved version of the repository `SIN-Language`, a compiler project that targeted a 16-bit VM. While this changes the compiler target, it also includes significant updates and improvements to elements that it retained from the original, such as the lexer and parser.

SIN is a custom procedural programming language based on C, C++, and Python, designed as an exercise in compiler design. It is named after the Spanish word meaning "without" because if anyone were to ask you about the language, you would reply that it is without any real utility or purpose. This repository contains a compiler for SIN with an x86 target; while I may add other architectures as I go on, x86 is the sole focus of the project for the time being.

## Goal of the Project

This project is a hands-on way of learning compiler development. The purpose of this project is not to create a particularly good compiler; it is a first attempt, and as such it will likely be mediocre at best. The goal is to serve as a learning exercise in compiler writing and solving various programming problems in the C++ language.

That said, I hope that this project will yield a functioning compiler, one that can be improved and expanded upon in future projects.

## Getting Started

### Installation / Build

There are currently no working binaries for the toolchain; you must compile and build it yourself. Note that this program is written to generate code for Linux on an x86-64 machine, and so it is unlikely that this compiler will generate usable programs on other platforms for the time being. A makefile is included in the project, but no working binaries will be.

### Third-Party Software

This is not a full compiler suite, and as such, a few third-party applications are required. Without them, the compiler will be not be able to generate working binaries for the programs you write. The required programs are:

* A working C compiler (such as GCC) - the compiler relies on certain C standard functions for things like memory allocation, so a C compiler with the standard library is necessary
* NASM - the compiler does not generate object code directly; instead, it generates x64 assembly in Intel syntax, meaning an assembler is required

### Using SIN

See the `doc/` folder for documentation on the language. I will also be utilizing GitHub Pages for a less technical introduction to actually using the compiler.

## Future Goals

I hope to use this project as a stepping stone to develop other languages and explore other features, such as compilers for object-oriented programming languages. For this project specifically, I hope to add in:

* Compiler optimizations
* Debugging support
* VSCode integration
* Native support for UTF-8 instead of ASCII
