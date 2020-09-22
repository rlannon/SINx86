# SIN Documentation

## Attributes

In SIN, unlike languages like C++ or Java, the `string` type is not implemented through a `struct` (or `class` in C++ and Java). As such, it doesn't really make sense to access the string's length this way; the dot operator is reserved for `struct` member access, and since types like `string` and `array` are not implemented through `struct`, we need something else. But, we want to avoid having a built-in `len()` function (or similar) because, following the SIN philosophy, all language features should be implemented syntactically, *not* through functions. Since we want accessing the length of a `string` or `array` to be a language feature (given they are built-in types, not implemented through a struct or template), and not a part of the standard library, we can use *attributes*.

An attribute is some information about an object that relies on its type. Attributes are either type-level, meaning it is information about a type, or value-level, meaning it is information about a specific object. This may change from object to object, or may be constant for a given type. All attributes yield `final`-qualified values, though some attributes will be evaluated at compile-time.

The syntax for attribute access is:

    value:attribute

So, you might say something like:

    // an example of value-level attributes
    alloc string s: "hello, world!";
    alloc int strlen &unsigned: s:len;
    @print("The length of the string is " + @itos(strlen)); // requires itos standard library function

    // and type-level ones
    alloc int INT_WIDTH: int:size;

The following attributes are currently available:

| Attribute | Return Type | Description |
| --------- | ----------- | ----------- |
| `len` | `unsigned int` | The number of elements in a collection; for `string` and `array`, this is contained at the head of the structure in a doubleword. For other types (`int`, `bool`, etc.), always returns 1. For `struct` types, returns the number of data members it contains |
| `size` | `unsigned int` | The number of *bytes* the data occupies. For a type like `float` or `unsigned short int`, equivalent to `sizeof< T >`. However, unlike `sizeof< T >`, the attribute can give the sizes of variable-width types |
| `var` | `unsigned int` | The variability of an object. Returns `2` for a variable, `1` for final data, and `0` for a constant |

Note that these attributes may be used on any value, including literal values, as all values have a type, and therefore, attributes. For example:

    alloc int x: 30:size;   // assigns 4
    alloc int y: "hello, world!":len;   // assigns 12

Note, though, that the compiler will issue a warning on lines 1 and 2 because we are assigning an `unsigned int` to a `signed int`, meaning we might get some data loss. This can be fixed with a typecast (e.g., `"hello, world!":len as signed int`).

## Other uses of the attribute operator

The attribute operator may also be used to select `static` elements from a class without referencing a specific object. For example, if we have the following struct `point`:

    def struct point {
        alloc static const int dimension: 3 &unsigned short;
        alloc int x;
        alloc int y;
        alloc int z;
    }

We could access `dimension` by saying `point:dimension` without referencing any specific object. We could also use an object with the dot operator as normal.
