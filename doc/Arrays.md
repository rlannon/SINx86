# SIN Documentation

## Arrays

Arrays in SIN operate in a similar manner as in Java; the array's length is tracked via a 32-bit integer at the array's head, and the data follows immediately after, allowing an array's length to be checked at runtime (beneficial for both static and dynamic arrays). Like arrays in other languages, SIN arrays have fixed lengths.

### Allocating an array

Whenever an array is allocated, the programmer must specify both the width _and_ the type contained within the array. The number of elements must be known at compile time unless dynamic memory is utilized. This looks like:

    alloc array<3, int> my_array;

This will allocate an array with 3 elements of type `int`. As mentioned, for dynamic arrays, the length need not be known at compile time. For example, the following:

    alloc int my_int: 10;
    alloc dynamic array<my_int, int>;

is completely valid. The array will be allocated like all other dynamic memory, but the number of requested bytes will be equal to:

    <number of elements> * <type width> + 4

to account for the cumulative size of each element plus the word for the number of elements.

Note that if we had _not_ marked the array as `dynamic`, a compiler error would have been generated as array lengths in these areas of memory must have `const` sizes (and `const` arrays must use initializer lists with alloc-init syntax). It is possible, however, to utilize named compile-time constants if the `constexpr` keyword is used:

    alloc const int my_int: 10;
    alloc static array<my_int &constexpr, int>;

The use of the `constexpr` keyword indicates to the compiler that the expression should be evaluated at compile-time (to save on compilation time, constants are not evaluated at compile-time unless the `constexpr` keyword is used). See the page on SIN constants for more information.
