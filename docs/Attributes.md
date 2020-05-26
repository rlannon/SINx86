# SIN Documentation

## Attributes

In SIN, unlike languages like C++ or Java, the `string` type is not implemented through a `struct` (or `class` in C++ and Java). As such, it doesn't really make sense to access the string's length this way; the dot operator is reserved for `struct` member access, and since types like `string` and `array` are not implemented through `struct`, we need something else. But, we want to avoid having a built-in `len()` function (or similar) because, following the SIN philosophy, all language features should be implemented syntactically, *not* through functions. Since we want accessing the length of a `string` or `array` to be a language feature, and not a part of the standard library, we can use *attributes*.

An attribute is some information about an object that relies on its type. This may change from object to object, or may be constant for a given type. The syntax for attribute access is:

    value:attribute

So, you might say something like:

    alloc string s: "hello, world!";
    alloc int strlen = s:len &unsigned;
    @print("The length of the string is " + strlen as string);

The following attributes are currently available:

| Attribute | Return Type | Description |
| --------- | ----------- | ----------- |
| `len` | `unsigned int` | The number of elements in a collection. For types like `int`, `bool`, etc., always returns 1. For `struct` types, returns the number of data members it contains |
| `size` | `unsigned int` | The number of *bytes* the data occupies. For a type like `float` or `unsigned short int`, equivalent to `sizeof< T >`. However, unlike `sizeof< T >`, the attribute can give the sizes of variable-width types |
| `var` | `string` | The variability of an object. Returns `var` for a variable, `final` for final data, and `const` for a constant |

Note that these attributes may also be used on *any* value, including literal values, as all values have a type, and therefore, attributes. For example:

    alloc int x: 30:size;   // assigns 2 (30 is a 'short int')
    alloc int y: "hello, world!":len;   // assigns 12
    alloc string s: 100:var;    // assigns "const"

Note, though, that the compiler will issue a warning on lines 1 and 2 because we are assigning an `unsigned int` to a `signed int`, meaning we might get some data loss.

## Other uses of the attribute operator

The attribute operator may also be used to select `static` elements from a class without referencing a specific object. For example, if we have the following struct `point`:

    def struct point {
        alloc static int dimension: 3 &unsigned short;
        alloc int x;
        alloc int y;
    }

We could access `dimension` by saying `point:dimension` without referencing any specific object. We could also use an object with the dot operator as normal.
