# SIN Documentation

## Types

### Overview

SIN is a strongly-typed language, and so all allocated memory must have a type declared along with it. All types have a default set of qualities (described in the table below) but they may be modified through the use of keywords. For example, an `int` is a signed, 32-bit integer, but may be made a 16-bit unsigned one by specifying it as an `unsigned short int`. Qualities may be applied to:

* **width** - `long` and `short` to double or halve the width, respectively; note that they may not be used on all types, and types like `float` may only support one
* **sign** - `signed` and `unsigned`; may only be used on `int` type
* **location** - `static` and `dynamic`; specifies where in memory the data should be allocated; in a function, all memory is allocated on the stack by default (except in the case of hidden pointer types), but the programmer may specify a different memory location with a keyword
* **variability** - `const` and `final` are used to indicate the variability of data; see the [relevant documentation](Constants.md) for more information on the difference
* **visibility** - `extern` is currently the only keyword marking visibility (currently, there is no `public` vs `private` distinction on struct members, though this is a planned addition), though `decl` statements will also alter data visibility

Here is a table containing relevant type information:

| Name | Width | Description | Valid qualifiers | Notes |
| ---- | ----- | ----------- | ---------------- | ----- |
| `int` | 32 bits | An integer number | width, sign, location, and variability | `signed` by default |
| `float` | 32 bits | A floating-point decimal number | width*, location, variability | May not be `unsigned`; may not be made `short` |
| `bool` | 8 bits | A boolean value; may be `true` or `false` | location, variability | `false` is stored as 0; `true` values may be any non-zero integer, though the literal `true` evaluates to 1 in code generation |
| `char` | 8 bits | A single ASCII character | location, variability | |
| `ptr<T>` | 64 bits | A pointer to type `T` | location, variability | `T` is a fully-parsed type that is the pointer's 'subtype' |
| `array<T>` | Variable | An array containing elements of type `T` | location, variability | Like pointers, arrays contain a fully-parsed subtype `T`. Further, SIN arrays contain the array's length. See the [documentation](Arrays.md) for more information |
| `string` | Variable | A string of ASCII characters | location, variability | SIN-strings use a 32-bit integer for the width followed by the appropriate number of ASCII characters. When strings are allocated, the program must allocate *at least* one extra byte and zero them out to allow the strings to be used with C (as C-strings are null-terminated) |
| `struct` | Variable | A user-defined type, more or less equivalent to a struct in C | location, variability | See the [documentation](Structs.md) for more information on structs in SIN |

You may note that [`array`](Arrays.md), [`string`](Reference%20Types.md), and [`struct`](Structs.md) may be of variable length. How this works changes based on the type; see the relevant documentation for more information.

### Subtypes

A few types in SIN require 'subtypes', meaning types that are contained by or pointed to by the type in question. These are fully-parsed so as to retain the language's type safety rules. For example, `ptr<int>` may not point to a `long int` because the types are of different widths; instead, you need a `ptr<long int>`. These types have to follow the type compatibility rules which include hierarchies that allow one-way relationships between certain types (including type promotion rules). See the document on [type compatibility](Type%20Compatibility.md) for more information.

### Typecasting

Not only is SIN a strongly-typed language, it does not allow implicit type conversions. As a result, it is the responsibility of the programmer to cast expressions to the proper type. SIN uses Rust-style typecasting with the `as` keyword. All primitive types can be cast to most other primitive types, but some conversions require standard library functions.

For more information, see the [relevant document](Typecasting.md).
