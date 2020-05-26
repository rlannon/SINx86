# SIN Documentation

## Assignment and Movement

SIN supports two keywords for variable data assignment, and while they are often interchangeable, they perform slightly different functions when used with references and dynamic data.

### Assignment

Assignment in SIN is performed with the `let` keyword. For example,

    let x = y;

copies the value in `y` to the location `x`. This is important -- `let` *copies* data. As such, something like:

    alloc array<3, int> my_arr: { 0, 1, 2 };
    alloc array<int> s2 &dynamic;
    let s2 = s;

`s2` will be reallocated to the size of `s`, and `s` will then be *copied* into `s2`. It results in two distinct, but identical, objects in dynamic memory.

### Movement

Sometimes, we want to *move* data instead of *copying* it, especially if we have large dynamically-allocated objects. For non-reference types, this ultimately simply copies the data, but if we are utilizing references or dynamic memory, we can move the data by copying its reference into a new location. For example:

    construct some_large_struct s (10, 20, 30) &dynamic;    // dynamically allocate and initialize a struct
    alloc some_large_struct a_copy &dynamic;
    move s -> a_copy;

We can also simply do this by using the `ref< T >` type:

    alloc ref<some_large_struct> r: $s;

### When to use `let` vs `move`

Sometimes, there is no difference between using `let` and using `move` on data. For example,

    alloc int a: 10;
    alloc int b;
    move a -> b;

Since `a` and `b` are not references, `move` is equivalent to `let`, and performs a copy.

Where the difference between them comes is when using things like dynamic arrays, as `let` allows data to be copied between arrays while `move` allows the reference to be moved and array to be reallocated. For example:

    alloc array<10, int> a &dynamic = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    alloc array<50, int> b &dynamic;
    let b = a;  // copy the elements into the new array
    move b -> a;    // move the array, updating the reference

In the above code sample, we first allocate an array of 10 elements and initialize it. This array is `dynamic`, meaning it can be reallocated. We then allocate a larger dynamic array, `b`, and copy all of the elements from `a` into it. Once this reference is copied, the reference count for the original chunk of data allocated by `a` hits 0, making it inaccessible, resulting in it being cleaned up by the SRE [Memory Allocation Manager](Memory%20Allocation%20Manager.md).
