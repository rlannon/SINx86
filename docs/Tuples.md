# SIN Documentation

## Tuples

Tuples are a first-class type in SIN and are roughly equivalent to tuples in other languages. They are ordered list types, like arrays, but unlike arrays, tuples may be heterogeneous. They may contain any fixed-width type, meaning contained arrays must have `constexpr` widths. In this regard, they are similar to structs; they may have an arbitrary number of elements of any fixed-width type, but their fields are all anonymous. Instead of being referenced by name, they are referenced with an index value (as with arrays).

### Creating a tuple

Tuples are allocated as the type

    tuple< T0, T1, T2, ... Tn >

Where each `T` is the type of the subsequent element. For example:

    alloc tuple<int, float, string> tup;

Tuples may utilize list initializers, like arrays, but they utilize parentheses around their elements instead of curly braces. For example:

    let tup = (0, 1.234, "hello, world!");

In such a case, each type within the tuple list must be the same as the type within the tuple itself or a `TypeError` will be generated.

### Accessing a tuple's members

Tuple member accession is done with the _dot operator_ and an integer literal like in Rust, not the index operator with an integer expression like in Python. For example:

    alloc int i: tup.0; // OK
    alloc float f: tup[1];  // syntax error; tuple may not be indexed with the [] operator

This allows compile-time bounds- and type- checks on tuple members. As a result, the tuple does not contain a word indicating its length in memory because its length and size are known at compile time.

### Tuple attributes

Tuples support the following attributes:

* `len` - the number of elements the tuple contains
* `size` - the number of bytes the tuple occupies
* `var` - the variability of the object
