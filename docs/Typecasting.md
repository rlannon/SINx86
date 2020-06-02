# SIN Documentation

## Typecasting

SIN, like Rust, allows typecasting with the `as` keyword. However, there are some rules regarding which types can be cast to and from other types.

`string`, `array`, and `ptr` types cannot be cast to and from one another, though individual elements of an `array` can be, provided they are a valid casting type.

For example, the following cast is valid:

    alloc array<3, int> my_arr: { 0, 1, 2 };
    alloc float my_float: my_arr[2] as float;

But saying something like this is not:

    alloc string my_string: "123";
    alloc int my_int: my_string as int;

Strings must be fully parsed in order to be converted to another type, which is not provided by the `as` keyword. Instead, a function like the standard library `itos` or `stoi` must be used.

However, conversion between types like `int` and `float` is okay; integers become floats with a fractional portion of 0, and floating-point numbers simply have their fractional portion removed. So:

    alloc int x: 30;
    alloc float f: x as float;  // f = 30.0
    alloc float pi: 3.14159;
    alloc int n: pi as int; // n = 3

Conversions to bool from numerics will yield `false` if equal to 0, and `true` for any non-zero number. Similarly, a value of `false` will yield a value of 0 and `true` will be 1 for integral types and 1.0 for floating-point numbers.

Casts may also be done to change or specify the width of a type, though decreasing a type's width might result in data loss. This will not generate a warning, as it is standard behavior. For example, mixing `unsigned int` and `signed int` will typically generate compiler warnings, but if typecasting is used, these warnings will go away. For example:

    alloc int x: 31;
    alloc int y: 415 &unsigned;
    
    let x = x & y;  // generates compiler warning about differing data widths

    alloc unsigned int z: x & y as unsigned int;    // no warning generated here
