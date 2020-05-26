# SIN Documentation

## Arrays

Arrays in SIN operate in a similar manner as in Java; the array's length is tracked via a 32-bit integer at the array's head, and the data follows immediately after. This allows an array's length to be checked at runtime, which is beneficial for both static and dynamic arrays. SIN arrays may have fixed or variable lengths, though this entirely depends on how the array is allocated. Automatic and static arrays are always fixed lengths, while arrays located on the heap allow dynamic lengths.

### Allocating an array

Whenever an array is allocated, the programmer must specify both the width and the type contained within the array, in that order (the type is `array< (N,) T >). The number of elements to be contained within the array must be known at compile time unless dynamic memory is utilized. This looks like:

    alloc array<3, int> my_array;

This will allocate an array with 3 elements of type `int`. As mentioned, for dynamic arrays, the length need not be known at compile time. For example, the following is completely valid:

    alloc int my_int: 10;
    alloc dynamic array<my_int, int>;

The array will be allocated like all other dynamic memory, but the number of requested bytes will be at least:

    <number of elements> * <type width> + 4

to account for the cumulative size of each element plus the word for the number of elements.

Note that if we had _not_ marked the array as `dynamic`, a compiler error would have been generated as array lengths in these areas of memory must have `const` sizes (and `const` arrays must use initializer lists with alloc-init syntax). It is possible, however, to utilize named compile-time constants if the `constexpr` keyword is used:

    alloc const int my_int: 10;
    alloc static array<my_int &constexpr, int>;

The use of the `constexpr` keyword indicates to the compiler that the expression should be evaluated at compile-time (to save on compilation time, constants are not evaluated at compile-time unless the `constexpr` keyword is used). See the page on SIN constants for more information.

Note that arrays may *not* contain other arrays, though they may contain *pointers* to arrays. For example:

    alloc array<5, array<int>> x;  // illegal
    alloc array<5, ptr<array<int>>> x; // legal way of doing this

Note that while arrays usually require the length, there are a few scenarios when it isn't:

* the array is a subtype of `ptr`; if a length is given, it will be ignored by the compiler (the programmer shall be notified this is the behavior by the compiler in a compiler note)
* the array is marked as `dynamic`; a length indicates how much initial memory should be reserved for the array, (possibly) preventing some of the overhead associated with reallocations. Note that if a length is not given, the array will have a size of 0 and the runtime bounds checks will prevent the array from being accessed (it must be reallocated)

Finally, arrays are always structured with the 32-bit length followed immediately by the array's elements, starting at 0. Regardless of where they are allocated, the length is at the lowest memory address and the final element is at the highest.
