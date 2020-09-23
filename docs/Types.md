# SIN Documentation

## Types

SIN is a strongly-typed language, and so all allocated memory must have a type declared along with it.

### Valid Data Types

Here is a table containing the types supported by SIN:

| Name | Width | Description | Valid qualifiers | Notes |
| ---- | ----- | ----------- | ---------------- | ----- |
| `int` | 32 bits | An integer number | width, sign, location, and variability | `signed` by default |
| `float` | 32 bits | A floating-point decimal number | width*, location, variability | May not be `unsigned`; may not be made `short` |
| `bool` | 8 bits | A boolean value; may be `true` or `false` | location, variability | `false` is stored as 0; `true` values may be any non-zero integer, though the literal `true` evaluates to 1 in code generation |
| `char` | 8 bits | A single ASCII character | location, variability | |
| `ptr<T>` | 64 bits | A pointer to type `T` | location, variability | `T` is a fully-parsed type that is the pointer's 'subtype' |
| `array<T>` | Variable | An array containing elements of type `T` | location, variability | Like pointers, arrays contain a fully-parsed subtype `T`. Further, SIN arrays contain the array's length. See the [documentation](Arrays) for more information |
| `string` | Variable | A string of ASCII characters | location, variability | SIN-strings use a 32-bit integer for the width followed by the appropriate number of ASCII characters. When strings are allocated, the program must allocate *at least* one extra byte and zero them out to allow the strings to be used with C (as C-strings are null-terminated) |
| `struct` | Variable | A user-defined type, more or less equivalent to a struct in C | location, variability | See the [documentation](Structs) for more information on structs in SIN |
| `tuple<T>` | Variable | A heterogeneous list of data | location, variability | Similar to arrays and structs. See the [documentation](Tuples) for specifics |

You may note that [`array`](Arrays), [`string`](Reference%20Types), and [`struct`](Structs) may be of variable length. How this works changes based on the type; see the relevant documentation for more information.

### Qualities

All types have a default set of qualities (described in the table below) but they may be modified through the use of keywords. For example, an `int` is a signed, 32-bit integer, but may be made a 16-bit unsigned one by specifying it as an `unsigned short int`. Qualities may be applied to:

* **width** - `long` and `short` to double or halve the width, respectively; note that they may not be used on all types, and types like `float` may only support one
* **sign** - `signed` and `unsigned`; may only be used on `int` type
* **location** - `static` and `dynamic`; specifies where in memory the data should be allocated; in a function, all memory is allocated on the stack by default (except in the case of hidden pointer types), but the programmer may specify a different memory location with a keyword
* **variability** - `const` and `final` are used to indicate the variability of data (compile- vs run-time constants), while `constexpr` indicates that a given expression may be evaluated at compile time; see the [relevant documentation](Constants) for more information on the differences
* **visibility** - `extern` is currently the only keyword marking visibility (currently, there is no `public` vs `private` distinction on struct members, though this is a planned addition), though `decl` statements will also alter data visibility; it may be applied to any global symbol

Qualities may be _prefixed,_ or placed before the type name, or _postfixed,_ placed after the information to which the type is being applied. Prefixed qualities always apply to the symbol or data immediately following the last qualifier while postfixed qualities always modify the symbol or data immediately before the first qualifier. To add a postfixed quality, use the ampersand (`&`) before the qualities to be added. Here is an example of how they are used:

    def int my_func &unsigned long () { ... }   // qualifiers come immediately after the symbol name
    alloc int x &unsigned;  // indicates that the type of 'x' has the 'unsigned' quality
    alloc unsigned int y;   // indicates the type we are declaring has the unsigned quality
    alloc int x: (10 + 20) &constexpr;  // indicates that the binary expression can be evaluated at compile time

### Subtypes

A few types in SIN require 'subtypes', meaning types that are contained by or pointed to by the type in question. The syntax for these subtypes is identical to C++ templates or Java generics. These are fully-parsed and exist to retain the language's type safety rules. For example, `ptr<int>` may not point to a `long int` because the types are of different widths; instead, you need a `ptr<long int>`. These types have to follow the type compatibility rules which include hierarchies that allow one-way relationships between certain types (including type promotion rules), typically relating to pointers and references. See the document on [type compatibility](Type%20Compatibility) for more information.

The `tuple` type may contain an arbitrarily-long list of contained types but may not be empty. Tuples are allowed to contain any type so long as its width can be determined at compile time.

### Typecasting

Not only is SIN a strongly-typed language, it does not allow implicit type conversions. As a result, it is the responsibility of the programmer to cast expressions to the proper type. SIN uses Rust-style typecasting with the `data as T` keyword rather than the C-style `(T)data` syntax. All primitive types can be cast to most other primitive types, but some conversions -- namely those which require string parsing -- require standard library functions.

For more information, see the [relevant document](Typecasting).
