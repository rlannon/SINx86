# SIN Documentation

## Assignment and Movement

SIN supports two keywords for variable data assignment, and while they are often interchangeable, they perform slightly different functions when used with references.

### Assignment

Assignment in SIN is performed with the `let` keyword. For example,

    let x = y;

copies the value in `y` to the location `x`. This is important -- `let` *copies* data. As such, something like:

    alloc array<3, int> my_arr: { 0, 1, 2 };
    alloc array<int> s2 &dynamic;
    let s2 = s;

`s2` will be reallocated to the size of `s`, and `s` will then be *copied* into `s2`. It results in two distinct, but identical, objects in dynamic memory.

### Movement

Sometimes, we want to *move* data instead of *copying* it, especially if we have large dynamically-allocated objects. For non-reference types, this ultimately simply copies the data, but if we are utilizing references, we can move the data by copying its reference into a new location. For example:

    construct some_large_struct s (10, 20, 30) &dynamic;    // dynamically allocate and initialize a struct
    alloc some_large_struct a_copy &dynamic;
    move s -> a_copy;

We can also simply do this by using the `ref< T >` type:

    alloc ref<some_large_struct> r: $s;
