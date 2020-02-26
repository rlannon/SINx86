# SIN Documentation

## Constants

SIN utilizes a few different keywords when talking about constant values, as there are two types of constants:

* Compile-time constants (like C++'s `const` or `constexpr`)
* Runtime constants (like Java's `final`)

The difference here is fairly subtle, but very significant; a compile-time constant in SIN *must* be assigned with a `constexpr` or a literal value whereas a runtime constant is one whose value may be unknown at compile time, but canot change after initialization. This is more akin to Java's `final` keyword, which indicates that the *reference* may not change once it is assigned.

In SIN, the words `const` and `final` are used for this distinction:

    alloc int a: 5;
    alloc const int b: 30;  // legal; a compile-time constant, assigned a constant when initialized
    alloc final int c: a;   // legal; a runtime constant, assigned only once

Note that only variables may be constant; functions must never be treated as constants because they may have side effects when called that will be as a result of their evaluation at compile time.

Due to the fundamental differences between the keywords, data may not be both `const` and `final` at the same time. Further, data may not be declared as both `dynamic` and `const`, as the two imply fundamentally different things (`final`, however, may be used with `dynamic`).

### The `final` keyword

The keyword `final` indicates that once the data is initialized, its value is final. `final` data are not required to use alloc-init syntax (as constants do).

### The `const` keyword

The keyword `const` indicates that the data will be read-only and must be known at compile-time. As such, it *must* be assigned using alloc-init syntax and may not be modified once initialized, *even through the use of pointers.* A constant may live in any memory area, as they may exist in any scope, though variables marked as `static const` may be stored in the program's `.rodata` segment instead of `.bss` or `.data`.

### The `constexpr` keyword

Just because `const` must use a compile-time constant in its initialization does not mean that constant must be a literal (although this is the simplest and most obvious compile-time constant expression); if a more complex expression is desired, the keyword `constexpr` may be placed before the expression (or suffixed like other qualifiers by using the ampersand, i.e., `&constexpr`) to indicate to the compiler that it should (attempt to) evaluate the expression at compile time. For example:

    alloc const int a: 5;
    alloc const int b: constexpr (a + 2); // valid use of constexpr
    alloc const int c: a + 2;   // illegal; constexpr not used

When the `constexpr` keyword is used, however, *all* data within must be known at compile time. As such, the following is illegal:

    alloc int a: 10;
    alloc const int b: 20;
    alloc const int c: constexpr (a + b); // illegal; a is not const-qualified

To save on compilation time, expressions are assumed to be known only at runtime (and therefore their evaluation is ignored by the compiler) unless the `constexpr` keyword is used, in which case the compiler will attempt to evaluate the expression and use the result in the generated code instead of generating code for the expression's evaluation. As such, a statement like:

    alloc static int a: constexpr 10 + 20;

will be compiled to:

    mov a, 30    

instead of something like:

    mov eax, 10
    add eax, 20
    mov a, eax

This allows the programmer to avoid magic numbers in code and save on compilation time at the same time by preventing the compiler from needlessly attempting to evaluate expressions.

#### A note on parsing

Note that the `constexpr` keyword indicates the expression to the *immediate* left or right is constant; this means something like:

    constexpr a + b

will be parsed such that the binary expression `a + b` is *not* constant, but the right operand `a` is. This allows partial evaluation of expressions at compile time. If you wish for an entire expression to be constant, use parens:

    constexpr (a + b)
    (a + b) &constexpr

The above examples are the proper ways of indicating the binary expression is a `constexpr`.
