# SIN Documentation

## Storage Specifiers

In SIN, like in many other programming languages, data may reside in one of 4 places:

* In program memory (or the `.text` and `.rodata` segments)
* In static memory, resolved at link time (the `.data` and `.bss` segments)
* On the stack (automatic memory; local variables)
* On the heap (dynamically-allocated memory)

Where data is stored is more or less up to the programmer. Like in languages like C and C++, programmers have a lot of control over where their variables and constants live. However, there are also rules to using different areas of memory and what you are and aren't allowed to do in each.

### Static Memory

Static memory, resolved at link time and specified with the `static` keyword, will be allocated exactly once and is available from anywhere in the program (though it may need to be accessed via a reference). The only memory that a `const ptr` may address is memory that is marked `static`, as this is the only area of memory where the compiler knows where the data lives before the program is run.

When allocating static memory, it will typically be placed in the `.data` or `.bss` segments of the object code, dependent upon whether or not the variable was initialized. Data marked as `static const` may be placed in the `.rodata` segment instead, as it will not change and can therefore live in read-only memory. However, regardless of whether or not the static data is marked `const`, if the data is to be initialized in the allocation, the initial value must _always_ be a compile-time constant. This is because the initial value will utilize NASM's `db` (or `dw`, `dd`, or `dq`, depending on the length) directive instead of making the assignment at runtime.

While local data may be declared `static` (generating a compiler warning, as this is usually unnecessary and a waste of space), any variables declared outside of a function or structure will be `static` automatically, though that does not necessarily mean global (see ).

### Automatic Memory

Automatic memory is memory that utilizes the stack. Like C, automatic memory is the default storage specifier and utilizes no special keywords. Automatic memory may be marked `const` or `final`.

### Dynamic Memory

Dynamic memory utilizes the `dynamic` keyword and causes the data to be allocated on the heap at run time. `dynamic` data, due to its nature of having a location and value knowable only at runtime, cannot be marked as `const`; instead, `final` should be used for `dynamic` data that the programmer does not want altered.
