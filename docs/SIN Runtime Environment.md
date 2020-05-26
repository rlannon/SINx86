# SIN Documentation

## SIN Runtime Environment

The SIN Runtime Environment (SRE) is a small library that is required by every SIN program. The purpose of the library is to enable certain features of the language that may be invoked when performing operations such as string assignments, array copying, or managing dynamically-allocated memory. In other words, it provides certain runtime functionality for the program required to implement all of its features. Note that this is different from the SIN Standard Library; unlike the SL, the SRE is required for all SIN programs to run, while the SL just provides useful tools to the programmer without being strictly required for execution. Further, the SRE not only contains subroutines, but reserves certain memory buffers for SIN programs that are required for the language to be fully functional. If the SRE is ommitted from the project, build will fail at link time because the compiler assumes the SRE subroutines and data will be available.

A copy of the SIN runtime must be included with a SIN compiler for it to be considered complete, though a user could opt to write their own if they so desired. Further, while these subroutines are *not* intenteded to be used in SIN, a programmer could declare them in the program in order to use them that way, though that would be incredibly inconvenient.

Some functions within the SRE provide runtime error handling. While SIN, like C, does not contain support for runtime exceptions, it has limited runtime error support. One of the crucial areas where this runtime error support is used is in bounds-checking on arrays at runtime. Unlike arrays in C or C++, the `array` type in SIN contains its width alongside the data contained within it, allowing the program to check that all array accession is done within the array's bounds; this is to make the language a little bit safer and easier to use, as allowing the programmer to access the array's length at runtime saves the headache of, for example, passing array length as a parameter when using them in conjunction with functions. Because a runtime array access might be illegal, the language requires that there be a routine available at runtime that will catch the error and alert the user.

This document is to serve as a reference for the built-in runtime environment. Note that much of the SRE's functionality utilizes functions from the C standard library (such as `malloc` and `free`), which is why the SIN compiler requires a working copy of your C compiler of choice to create executables. A compiled copy of the SRE is not included in this project, but a copy of its source can be found [here](https://github.com/rlannon/SRE).

## The Memory Allocation Management System (MAM)

In SIN, memory management is typically done with the `alloc` and `free` keywords in combination with location specifiers like `dynamic`. Unlike C, these are done with *keywords* instead of library functions. However, this means that some library support is still required, even though it is not visible to the programmer. Rather, it is done automatically by SIN's memory management system, called the Memory Allocation Manager, or MAM.

For more information on the MAM, see [this document](Memory%20Allocation%20Manager.md).

## The SRE

The SRE is divided into a series of modules, divided up by the role they serve in the library. All functions in the SRE are prefixed with `sinl_` to indicate they are a SIN language function.

Note that all subroutines in the SRE utilize the `sincall` calling convention, though some features of this convention are available in the assembly that are not available in SIN (such as secondary return values).

### The `memory` module

The `memory` module contains all of the necessary subroutines for SIN's automatic memory management (such as the `dynamic` and `free` keywords).

#### `sinl_malloc`

`sinl_malloc` implements the `dynamic` keyword. It attempts to allocate a specific amount of memory for the user and returns a pointer to it. This is implemented through the use of C's `malloc()` (or `calloc()`).

This subroutine takes the following parameters:

* `len` - A 32-bit integer containing the desired length, in bytes, for the allocated memory

The following values are returned from the subroutine:

| Register | Description | Notes |
| -------- | ----------- | ----- |
| `rax` | The address of the allocated memory | `null` if the allocation failed |
| `rbx` | The number of bytes actually allocated | Secondary return value |

#### `sinl_free`

`sinl_free` implements the `free` keyword. This decrements the reference count of the resource at the specified location, freeing it if the count reaches zero. This is ultimately uses the C function `free()` to free the memory.

This subroutine takes the following parameters:

* `addr` - The address of the memory to be freed

When the subroutine returns, `rax` will contain a boolean value indicating whether the free was successful (1) or not (0). Note, however, that if the free operation is unsuccessful, it is likely that a system fault will be triggered and execution will be terminated.

### The `string` module

The SRE has a few functions that are utilized by the compiler to handle various string operations. Without this module, use of the `string` data type would not be possible.

#### `sinl_str_cpy`

`sinl_str_cpy` implements the `let` keyword for strings. The subroutine copies the data (including the length word) from one area of memory to another.

This subroutine takes the following parameters:

* `src` - A pointer to the source string
* `dest` - A pointer to the destination string

The function returns no value.

#### `sinl_str_concat`

`sinl_str_concat` implements string concatenation (operator `+`) through a combination of arithmetic, data copies, and the `sinl_buffer` allocated to each program.
