# SIN Documentation
## Pointers

Like C and C++, SIN allows programmers to use *pointers* to access data. This is particularly useful, for example, when one wishes to pass large objects as a function parameter to save space on the stack. Note that unlike C, however, SIN makes use of the `dynamic` keyword to indicate heap allocation rather than a function like `malloc`. The result is a pointer in the object code, but syntactically, the data is not treated any differently than stack-allocated variables. However, any variable allocated dynamically must be freed lest a memory leak occurs.

Pointers in SIN are very explicit about separating the qualities of the pointer from the qualities of the data to which the pointer is pointing. Note, though, that the only qualities that may be applied to a pointer are `const`, `final`, `static`, and `dynamic`; anything else, i.e. width and sign specifiers, will be ignored (as a pointer is always a 64-bit unsigned integral type in SIN).

### ```const``` pointer to ```int``` vs pointer to ```const int```

Pointers in SIN are much more explicit than C about the type of data to which they point as well as how the pointer itself is to be stored and accessed. Take the following examples from C:

    const int *a;
    int const *b;
    int *const c;

In the above examples, we have two different types of pointers present:

* Pointers to a ```const int``` (`a` and `b`)
* And a ```const``` pointer to ```int``` (`c`)

The syntax for the difference leaves much to be desired; and so in SIN, this is handled much more explicitly in the syntax:

    alloc ptr<const int> a;   // equivalent to 'const int *a' or 'int const *a'
    alloc ptr<int &const> b;    // the same
    alloc const ptr<int> c; // equivalent to 'int *const c'
    alloc const ptr<const int> d;

The above example takes it one step further -- like the C example, ```a``` and ```b``` are pointers to ```const int```s, while ```c``` is a ```const``` pointer to an ```int```. The data ```d```, however, is a ```const``` pointer to a ```const int```. That is to say, for ```a``` and ```b```, the reference may be changed, but the pointed-to data may not; ```c``` allows the data to be changed, but not the reference; and ```d``` allows neither.

### Type Promotion

As a result of this ability to have pointers to constants, constant pointers, and constant pointers to constants, as well as the difference between compile-time and runtime constants, a hierarchy emerges in terms of what has the most "weight." This hierarchy, in descending order, is:

1. ```const```
2. ```final```
3. Neither

How this plays out in SIN is that a pointed-to type can always *promote*, but never *demote*, whatever it is pointing to. For example:

    alloc int my_const: 30 &const;
    alloc int my_final: 40 &final;
    alloc int my_plain: 50;

    alloc ptr<const int> my_const_pointer;
    alloc ptr<final int> my_final_pointer;
    alloc ptr<int> plain_old_pointer;

    let my_const_pointer = $my_plain;   // OK; temporarily promotes 'my_plain' to 'const' when referenced through 'my_const_pointer'
    let my_final_pointer = $my_final;   // OK; final = final
    let my_const_pointer = my_final;    // OK; promotes 'my_final' to 'const'
    let my_final_pointer = my_const_pointer;    // Illegal; cannot demote 'my_const_pointer' from 'const' to 'final'
    let plan_old_pointer = my_final_pointer;    // Illegal; cannot demote 'my_final_pointer' from 'final' to non-final, non-const

In the above example, we can see that when we assign the address of non-const, non-final data to a pointer to a constant, that non-const, non-final data gets 'promoted' such that when accessed via the pointer, it is treated as having more restrictive access. Note that when accessed normally (not through a pointer), it is treated as normal; it only gets *temporarily* promoted when accessed via the pointer.

That also means that we are explicitly forbidden from "demoting" data from `const` or `final` to anything less restrictive.
