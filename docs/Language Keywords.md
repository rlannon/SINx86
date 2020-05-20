# SIN Documentation

## Language Keywords

This document is meant to serve as an overview to all of the SIN language keywords. They are divided up into sections according to their purpose.

## Statement Keywords

The following keywords are those which can be used to begin statements.

### Symbols

* `def` - defines a block of code to be associated with a [function](Functions.md) or [struct](Structs.md)
* `alloc` - allocates data of a given type
* `free` - frees the given memory back to the environment
* `let` - assignment keyword

### Control Flow

* `if` - defines a condition that, if true, will result in the following statement or scoped block to be executed
* `else` - declares a block to be executed if the condition is not true
* `while` - defines a condition that, while true, will cause the execution of the following statement or scoped block

### Miscellaneous

* `asm` - inline assembly
* `pass` - equivalent to Python's `pass` - does nothing (but not necessarily a `NOP` - the compiler may simply ignore it)

## Expression Keywords

The following keywords may *not* be used to begin a statement, but may be required as additional information to statements (e.g., type information in allocations).

### Operators

* `and` - logical and
* `or` - logical or
* `xor` - logical exclusive-or
* `not` - logical not

### Data Types

#### Fundamental Types

* `int` - integral types (default 32-bit)
* `float` - floating-point types (default single-precision, 32-bit)
* `char` - a single character
* `string` - a string type
* `struct` - a user-defined struct type
* `ptr` - a 64-bit pointer type
* `array` - a homogeneous array of data

#### Width and Sign

* `long` - doubles the data's width (only for numeric types)
* `short` - halves the width (only for numeric types)
* `unsigend` - specifies the data should be unsigned
* `signed` - specifies the data should be signed

#### Variability and Location Specifiers

* `const` - compile-time constants (value known at compile time)
* `final` - runtime constants (may not be modified once assigned)
* `static` - data that should be allocated at compile time
* `dynamic` - data that should be allocated at runtime on the heap; may be, but does not have to be, variable-width; may not be `const`, but may be `final`
* `constexpr` - specifies an expression may be evaluated at compile-time (uses only literals and `const` data)
