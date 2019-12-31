# SIN-Language

SIN is a custom procedural programming language based on C, C++, and Python, designed as an exercise in computer language design. It is named after the Spanish word meaning "without" because if anyone were to ask you about the language, you would reply that it is without any real utility or purpose. This repository contains a compiler for SIN with an x86 target; while I may add other architectures as I go on, x86 is the sole focus of the project for the time being.

## Goal of the Project

This project is a hands-on way of learning compiler development. The purpose of this project is not to create a particularly good compiler; it is a first attempt, and as such it will likely be mediocre at best. The goal is to serve as a learning exercise in compiler writing and solving various programming problems in the C++ language.

That said, I hope that this project will yield a functioning programming language compiler, one that can be improved and expanded in future projects.

## Getting Started

### Installation / Build

There are currently no working binaries for the toolchain; you must compile and build it yourself. Note that this program is written for Linux on an x86_64 machine, and as such, any other build spec will likely fail miserably. A makefile is included in the project, but no working binaries will be.

### Using SIN

I will be posting the documentation for the language on github pages. Stay tuned.

## Future Goals

I hope to use this project as a stepping stone to develop other language and explore other features, such as compilers for object-oriented programming. For this project specifically, I hope to add in:
* Compiler optimizations
* Debugging support
* VSCode integration
