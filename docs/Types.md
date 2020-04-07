# SIN Documentation

## Types

### Overview

SIN is a strongly-typed language, and so all allocated memory must have a type declared along with it. All types have a default set of qualities (described in the table below) but they may be modified through the use of keywords. For example, an `int` is a signed, 32-bit integer, but may be made a 16-bit unsigned one by specifying it as an `unsigned short int`. Qualities may be applied to:

* **width** - `long` and `short` to double or halve the width, respectively; note that they may not be used on all types, and types like `float` may only support one
* **sign** - `signed` and `unsigned`; may only be used on `int` type
* **location** - `static` and `dynamic`; specifies where in memory the data should be allocated; in a function, all memory is allocated on the stack by default (except in the case of hidden pointer types), but the programmer may specify a different memory location with a keyword
* **variability** - `const` and `final` are used to indicate the variability of data; see the [relevant documentation](Constants.md) for more information on the difference

Here is a table containing relevant type information:

| Name | Width | Description | Valid qualifiers | Notes |
| ---- | ----- | ----------- | ---------------- | ----- |
| `int` | 32 bits | An integer number | width, sign, location, and variability | `signed` by default |
| `float` | 32 bits | A floating-point decimal number | width*, location, variability | May not be `unsigned`; may not be made `short` |
| `bool` | 8 bits | A boolean value; may be `true` or `false` | location, variability | `false` is stored as 0; `true` values may be any non-zero integer, though the literal `true` evaluates to 1 in code generation |
| `char` | 8 bits | A single ASCII character | location, variability | |
| `ptr<T>` | 64 bits | A pointer to type `T` | location, variability | `T` is a fully-parsed type that is the pointer's 'subtype' |
| `array<T>` | Variable | An array containing elements of type `T` | location, variability | Like pointers, arrays contain a fully-parsed subtype `T`. Further, SIN arrays contain the array's length. See the [documentation](Arrays.md) for more information |
| `string` | Variable | A string of ASCII characters | location, variability | SIN-strings use a 32-bit integer for the width followed by the appropriate number of ASCII characters |
| `struct` | Variable | A user-defined type, more or less equivalent to a struct in C | location, variability | See the [documentation](Structs.md) for more information on structs in SIN |

You may note that `array`, `string`, and `struct` are of variable length. See the relevant [documentation](Hidden%20Pointer%20Types.md) for more information on how this works.

### Subtypes

A few types in SIN require 'subtypes', meaning types that are contained by or pointed to by the type in question. These are fully-parsed so as to retain the language's type safety rules. For example, `ptr<int>` may not point to a `long int` because the types are of different widths; instead, you need a `ptr<long int>`. These types have to follow the type compatibility rules which include hierarchies that allow one-way relationships between certain types (including type promotion rules). See the document on [type compatibility](Type%20Compatibility.md) for more information.

### Typecasting

Not only is SIN a strongly-typed language, it does not allow implicit type conversions. As a result, it is the responsibility of the programmer to cast expressions to the proper type. This means that, for example, the following code would work in C:

    int x = 10;
    if (x) {
        printf("true");
    }

its SIN counterpart would not:

    alloc int x: 10;
    if (x) {                // error: expected 'bool', found 'int'
        @print("true");
    }

You would have to rewrite it as:

    alloc int x: 10;
    if ( x != 0 ) {         // alternatively, one could use the function 'itob' to cast
        @print("true");
    }
