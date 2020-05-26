# SIN Documentation

## References

SIN, like C++, actually contains two pointer types -- raw pointers, implemented through the `ptr< T >` type, and references, implemented through the `ref< T >` type as well as the `dynamic` keyword. Unlike pointers, references in SIN cannot be `null`, and must *always* be initialized through alloc-init syntax (e.g., `alloc ref<int> x: $y`). They are `final` by default, but can be made `const` (note that adding the `final` keyword makes its variability explicit, but is unnecessary).

SIN references, like C++ references, do not utilize any special syntax after they are allocated; unlike pointers, they do not need to be dereferenced by the programmer. However, unlike C++ references, and more similar to references in Rust, SIN references do require the address-of operator (`$`) to be used on the data they are referencing when initialized *if it is not initialized with another reference*. An example:

    alloc int x;
    alloc ref<int> r: $x;   // requires the address-of operator, as 'x' is not a reference
    alloc ref<int> r2: r;   // does not require the address-of operator, as 'r' is the type ref<int>
    let r = 3;
    @print(x as string);    // prints '3'