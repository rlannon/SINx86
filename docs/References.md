# SIN Documentation

## References

_Note: This document describes a future addition to the SIN language, one which is not currently present or in development within the compiler. As such, some of the details of its behavior may be adjusted._

SIN, like C++, actually contains two pointer types -- raw pointers, implemented through the `ptr< T >` type, and references, implemented through the `ref< T >` type as well as the `dynamic` keyword. Unlike pointers, references in SIN cannot be `null` and must *always* be initialized through alloc-init syntax (e.g., `alloc ref<int> x: y`). They are immutable, and so the reference may not be changed once created. This is akin to Java's `final` keyword -- the reference may not be updated. `const` may be applied to the reference type, and variability specifiers may be added to the referenced data.

SIN references, like C++ references, do not utilize any special syntax after they are allocated; unlike pointers, they do not need to be dereferenced by the programmer. Further, like C++, they do not use the address-of operator and must be initialized with the referent itself. For example:

    alloc int x: 10;
    alloc ref<int> r: x;

This was a deliberate choice to prevent a reference from ever being initialized with an invalid (null) value from raw pointers. If a pointer is to be used to initialize a reference, then we _must_ dereference it, and dereferencing a null pointer will result in a segmentation violation. This prevents the `ref< T >` type from ever containing an invalid reference.

Using the address-of operator on a `ref< T >` will result in a `ptr< T >`, not `ptr< ref< T > >`. Pointers to references (`ptr<ref<T>>`) are illegal; the address contained by the reference may not be modified. Like in C++, references in SIN should be thought of as aliases to the original referenced object.

For example:

    alloc int x: 10;
    alloc ref<int> r: x;
    alloc ptr<int> p: &r;   // &r = &x

Is completely valid. Here, `r` should be thought of as another name for `x`. Although the generated machine code will utilize pointers, syntactically, references don't behave like pointers. This is similar to how dynamic reference types like strings work -- they behave differently under the hood than they do syntactically/how they should be conceptualized.
