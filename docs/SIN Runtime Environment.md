# SIN Documentation

## SIN Runtime Environment

The SIN Runtime Environment (SRE) is a small library that is required by every SIN program. The purpose of the library is to enable certain features of the language that may be invoked when performing operations such as string assignments, array copying, or managing dynamically-allocated memory. In other words, it provides certain runtime functionality for the program that is required in order to implement all of SIN's features. Note that this is different from the [SIN Standard Library (SL)](https://github.com/rlannon/sinstdlib); the SRE is required for all SIN programs to run, while the SL just provides useful tools to the programmer without being strictly required for execution. Further, the SRE not only contains subroutines, but reserves certain memory buffers for SIN programs that are required for the language to be fully functional. If the SRE is ommitted from the project, build will fail at link time because the compiler assumes the SRE subroutines and data will be available.

A copy of the SIN runtime must be included with a SIN compiler for it to be considered complete, though a user could opt to write their own if they so desired. Further, while these subroutines are *not* intenteded to be used in SIN, a programmer could declare them in the program in order to use them that way, though that would be incredibly inconvenient.

Some functions within the SRE provide runtime error handling. While SIN, like C, does not contain support for runtime exceptions, it has limited runtime error support. One of the crucial areas where this runtime error support is used is in bounds-checking on arrays at runtime. Unlike arrays in C or C++, the `array` type in SIN contains its length alongside the data contained within it, allowing the program to check that all array accession is done within the array's bounds; this is to make the language a little bit safer and easier to use, as allowing the programmer to access the array's length at runtime saves the headache of, for example, passing array length as a parameter when using them in conjunction with functions. IT also provides an automatic check against [buffer overflow](https://en.wikipedia.org/wiki/Buffer_overflow). Because a runtime array access might be illegal, the language requires that there be a routine available at runtime that will catch the error and exit before data access can happen.

This document is to serve as a reference for the built-in runtime environment. Note that much of the SRE's functionality utilizes functions from the C standard library (such as `malloc` and `free`), which is why the SIN compiler requires a working copy of your C compiler of choice to create executables. A copy of the SRE is not included in this project, but its source [is available on GitHub](https://github.com/rlannon/SRE).

### The Memory Allocation Management System (MAM)

Perhaps the most important piece of the SRE is the Memory Allocation Manager (MAM). In SIN, memory management is typically done with the `alloc` keyword (and possibly `free`, although use of `free` is typically inadvisable) in combination with location specifiers like `dynamic`. Unlike C, all memory allocation is done with *keywords* instead of library functions. However, this means that some library support is still required, even though it is not visible to the programmer. Rather, it is done automatically by SIN's memory management system, called the Memory Allocation Manager, or MAM.

The MAM implements a garbage collector for the language by using reference counting on all dynamically-allocated resources. Every time a new reference or pointer references a dynamic object, the MAM increments its reference count; every time that pointer is reassigned, or the variable goes out of scope, the reference count is decremented. Once a resource's reference count hits zero, it becomes inaccessible, and the MAM automatically (and immediately) deletes it.

For more information on the MAM, see [this document](Memory%20Allocation%20Manager).

### Bounds Checking

As previously mentioned, the SRE helps with the implementation of automatic bounds checking on arrays and strings by providing error routines when out-of-bounds access attempts occur. These error routines will print an error message and immediately exit. While it is the programmer's responsibility to check that the access won't go out of bounds, the SRE ensures the program will still be memory-safe (unless the programmer really goes out of their way to circumvent the language's checks).
