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
* the array is marked as `dynamic`; a length indicates how much initial memory should be reserved for the array, (possibly) preventing some of the overhead associated with reallocations. Note that if a length is not given, the array will have a size of 0 and the runtime bounds checks will prevent the array from being accessed (it must be reallocated).

#### Memory structure and safety

Arrays are always structured with the 32-bit length followed immediately by the array's elements, starting at 0. Regardless of where they are allocated, the length is at the lowest memory address and the final element is at the highest. Whenever an array is accessed, the desired index is checked against the array's length to ensure the access is within the bounds of the array. Any attempt at accessing elements beyond the end or before the beginning of the array will cause the program to exit. However, this makes checking that the accession is within bounds easy via the array's `len` [attribute](Attributes.md).

The `=` assignment operator will always copy data from the source into the destination. For example, `let my_arr = another_arr` will copy _up to_ `my_arr:len` elements from `another_arr` into `my_arr`. Arrays do not have to be the same length for a copy to happen; if this is the case, the runtime will copy as many elements as it can from the source into the destination without stepping outside the bounds of either array. If `another_arr` is shorter, the 0th through `another_arr:len - 1` elements will be copied in.

Note that memory safety mechanisms for arrays still apply even when a `ptr<array>` is used.

### Arrays as function parameters

Arrays, like every other type, can be passed to functions as arguments. Depending on the qualities of the array, it may be passed on the stack or as a reference type to dynamic memory. The qualifications are as follows:

#### Fixed-length arrays

Arrays whose length is known at compile-time may be passed on the stack directly. Their data will be copied from the source into the destination, just as any other array, and will be treated as a local array. However, in order for arrays to be passed like this, they must declare their length in the function definition. Arrays of different lengths may be passed to the function, and like other arrays, it will copy as many elements as it can from the source into the destination. A sample function definition that uses this method would look like:

    def void my_func(alloc array<10, int> arr) { ... }

#### Dynamicall-sized (`dynamic`) arrays

If a dynamicall-sized array is desired, the `dynamic` keyword may be used on the argument. For example:

    def void my_func(alloc dynamic array<int> arr) { ... }

This will create a _new_ resource and copy the data from the source (argument supplied) into the space pointed to by `arr`. This can be wasteful, so if the array doesn't need to be modified, consider using `ptr<final array>` instead.

However, a dynamically-sized array is not resizeable; once initialized, their lengths, like all arrays, are fixed. For dynamic arrays as struct members, an allocation expression _(note: not yet implemented)_ may be used to specify the length. If a dynamic array is allocated, but its width unspecified, the SRE allocation will be delayed until an assignment is made, in which case the MAM will allocate an array of the proper size (the size of the array being copied) on the heap and perform the copy. For example:

    alloc array<5, int> a: {0, 1, 2, 3, 4};
    alloc array<int> b &dynamic;    // allocation is deferred
    let b = a; // allocates array 'b' with length 5 on the heap

If you wish to have arrays that you can resize, you must implement this yourself.

An example of dynamically-sized arrays as function parameters is in the program's entry point, `main`. The proper signature for `main` is:

    def int main(alloc dynamic array<string> args) { ... }

_Note:_ the compiler will allow a return type other than `int`, though a warning will be generated. It will also allow a definition of `main` with no parameters, but any parameters aside from a `dynamic array<string>` will be rejected at compile-time.

#### Pointers to arrays

Finally, a pointer to an array can be given, which may point to any array, fixed-length or not. Like other pointers, modifications to the data at the pointer in the scope of the function will modify the original, as a copy is not made.
