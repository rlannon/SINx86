# SIN Documentation

## Pointers

Like C and C++, SIN allows programmers to use *pointers* to access data. This is particularly useful, for example, when one wishes to pass large objects as a function parameter to save space on the stack. Note that unlike C, however, SIN makes use of the `dynamic` keyword to indicate heap allocation rather than a function like `malloc`. The result is a pointer in the object code, but syntactically, the data is not treated any differently than stack-allocated variables. However, any variable allocated dynamically must be freed lest a memory leak occurs.

Pointers in SIN are very explicit about separating the qualities of the pointer from the qualities of the data to which the pointer is pointing. Note, though, that the only qualities that may be applied to a pointer are `const`, `final`, `static`, and `dynamic`; anything else, i.e. width and sign specifiers, will be ignored (as a pointer is always a 64-bit unsigned integral type in SIN).

### `const` pointer to `int` vs pointer to `const int`

Pointers in SIN are much more explicit than C about the type of data to which they point as well as how the pointer itself is to be stored and accessed. Take the following examples from C:

    const int *a;
    int const *b;
    int *const c;

In the above examples, we have two different types of pointers present:

* Pointers to a `const int` (`a` and `b`)
* And a `const` pointer to `int` (`c`)

The syntax for the difference leaves much to be desired; and so in SIN, this is handled much more explicitly in the syntax:

    alloc ptr<const int> a;   // equivalent to 'const int *a' or 'int const *a'
    alloc ptr<int &const> b;    // the same
    alloc const ptr<int> c; // equivalent to 'int *const c'
    alloc const ptr<const int> d;

The above example takes it one step further -- like the C example, `a` and `b` are pointers to `const int`s, while `c` is a `const` pointer to an `int`. The data `d`, however, is a `const` pointer to a `const int`. That is to say, for `a` and `b`, the reference may be changed, but the pointed-to data may not; `c` allows the data to be changed, but not the reference; and `d` allows neither.
