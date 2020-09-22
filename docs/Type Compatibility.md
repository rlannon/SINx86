# SIN Documentation

## Type Compability

In SIN, there are fairly strict rules surrounding type compatibility in SIN due to its strongly-typed nature. To understand how type compatibility operates in SIN, one must first understand how SIN types are structured.

All SIN types are composed of a *primary type,* a *width,* a *sign,* *storage, variability, visibility, and layout specifiers,* and possibly a *subtype* which is itself a complete type. All of these factors play into whether two types are compatible. Further, whether types are compatible depends on the order they appear (type `A` and type `B` may be compatible such that `let A = B` is valid, but `let B = A` is not); this is typically due to the variability of each type.

## Rules

Here is a general breakdown of the rules regarding type compatibility. They will be followed with examples.

### Primary Type Comparison

In order for two types to be comptible, their primary types must be compatible. This means that one of three things must be true:

* The primary types are the same *and* secondary types are compatible
* One of the primary types is `raw< N >`

Note that this does not take into account whether you are using an operator like `$` or `[]`; these operators change the expression type, and so are evaluated on the basis of the operator's return type *and* the symbol's type, not the symbol's type alone.

### Width Comparison

Types of differing widths can be compatible, but a warning will be generated indicating a loss of data is possible when converting to a type with a smaller width than the current one (e.g., trying to store a `long int` in an `int`).

### Sign Comparison

Signed and unsigned types are considered compatible, but like differing-width types, a compiler warning will be generated indicating that there is a possible data loss when the two types interact. Note that an expression will be considered signed if either of these conditions are met:

* subtraction is present in the expression
* any of the data members are signed

### Storage

Storage specifiers specify _where_ in memory some data lives. There are three (or four) areas of memory where data can reside:

* The stack (automatic memory; no specifier)
* The heap (dynamic memory; utilizes `dynamic`)
* In the `.data` segment (allocated at compile-time; utilizes `static`)
* In the `.rodata` segment (allocated and computed at compile-time; utilizes `static const`)

### Variability

Because of the variability differences between types (e.g., `const` or `final` data), as well as how these types may be used in pointers, a hierarchy emerges in terms of what has the most "weight." This hierarchy, in descending order, is:

1. `const`
2. `final`
3. Neither `const` nor `final`

The rule with type promotion is that a type may be *promoted*, but it may *never* be *demoted*; and a data member may always be initialized with a data member *higher* than it in the hierarchy. This means that when compatibility is being checked, a `const` must never be treated as a `final` or variable type, though a variable or `final` type may be treated as `const`. Take the following example:

    alloc int a: 30 &const;
    alloc int b: a &final;  // legal; final data may be initialized with a constant
    alloc int c: b &const;  // illegal; const may only be initialized with other const data, and 'b' is not

As we can see in the above example, data members may only be initialized with data that is equal to or above it in the hierarchy.

#### Type Promotion

The promotion rule typically applies to use of such data in conjunction with pointers. When pointed to, the pointed-to data must be compatible within the hierarchy. The pointed-to data is *always* the data that we try to promote to the variability of the *ptr* type. In other words, when we access data via a pointer, the variability specified by that pointer *will never change*, and as such, the pointer will treat all data that it points to as being that type. This means if we have a pointer that expects a constant, any data that it points to will be treated as a constant whenever that pointer is used. This rule prevents us from modifying `final` and `const` data through pointers. An example of this in action:

    // allocate some data
    alloc int my_const: 30 &const;
    alloc int my_final: 40 &final;
    alloc int my_plain: 50;

    // allocate some pointers
    alloc ptr<const int> my_const_pointer;
    alloc ptr<final int> my_final_pointer;
    alloc ptr<int> plain_old_pointer;

    // make pointer assignments
    let my_const_pointer = $my_plain;   // OK; promotes 'my_plain' to 'const'
    let my_final_pointer = $my_final;   // OK; final = final
    let my_const_pointer = my_final;    // OK; promotes 'my_final' to 'const'

    let my_final_pointer = my_const_pointer;    // Illegal; cannot demote 'my_const_pointer' from 'const' to 'final'
    let plan_old_pointer = my_final_pointer;    // Illegal; cannot demote 'my_final_pointer' from 'final' to non-final, non-const

In the above example, we can see that when we assign the address of non-const, non-final data to a pointer to a constant, that non-const, non-final data gets 'promoted' such that when accessed via the pointer, it is treated as having more restrictive access. Note that when accessed normally (not through a pointer), it is treated as normal; it only gets *temporarily* promoted when accessed via the pointer. However, when we attempt to point to a `const int` via a `ptr<final int>`, it fails because a constant may not be demoted to `final` status.

That also means that we are explicitly forbidden from "demoting" data from `const` or `final` to anything less restrictive.

### Visibility

By default, data defined in a particular source file may only be accessed from within that file; it has local visibility. However, there are two ways data can be made available globally -- either with the `extern` keyword or by using a `decl` statement. Data that is made available with `extern` will not undergo name mangling, and therefore will be available to, for example, a C program. Data that utilizes `decl` will still undergo name mangling, and will be available to other source files. It also allows for forward-declaration of data.

See [the document on includes](Includes) for more information on why `decl` is important in included files.
