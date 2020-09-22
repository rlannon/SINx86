# SIN Documentation

## Pointers

Like C and C++, SIN allows programmers to use *pointers* to access data. This is particularly useful, for example, when one wishes to allocate large objects so as to allocate stack space. Note that unlike C, however, SIN makes use of the `dynamic` keyword to indicate heap allocation rather than a function like `malloc`, and so use of `dynamic` is usually preferable to `ptr` in such cases. The result is a pointer in the object code, but syntactically, the data is not treated any differently than stack-allocated variables (`static dynamic` and `(static) const dynamic` data are, by definition, impossible). `dynamic` data may be passed out of scope with a `ptr` or a [reference](References).

Pointers in SIN are very explicit about separating the qualities of the pointer from the qualities of the data to which the pointer is pointing. This allows us to retain type and variability safety (a pointer to a `const int` will not be able to modify the pointed-to data, nor will a pointer to an `int` ever point to a `const int` -- it violates the variability policy). Note, though, that the only qualities that may be applied to a pointer are `const`, `final`, `static`, and `dynamic`; anything else, i.e. width and sign specifiers, will be ignored (as a pointer is always a 64-bit unsigned integral type in SIN) and generate a compiler note.

The syntax for pointers is `ptr< T >` where `T` is a fully-parsed subtype. All pointers must be assigned with other pointers or with the address-of operator (`$`). Pointers are dereferenced with the dereference operator (`*`), which will yield data of type `T`. This allows pointers to be nested indefinitely while still retaining full type safety. For example:

    alloc int a: 10;
    alloc ptr<int> p: $a;
    
    alloc int b: *a;    // initialize integer with dereferenced pointer
    alloc ptr<int> p2: p;   // initialize pointer with another pointer

    alloc ptr<ptr<int>> p3: $p; // initialize a pointer to a pointer
    let p2 = *p3;   // dereferencing gives us data of the type ptr<int>

### `const` pointer vs pointer to `const`

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

**NB:** If you have a `const ptr` to `int`, that integer effectively needs to be `static`; a `const ptr` must know its pointed-to value at compile time, which means the *location* of the pointed-to data must be known by link time (i.e. it must be a named constant).
