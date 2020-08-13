# SIN Documentation

## Typecasting

SIN, like Rust, allows typecasting with the `as` keyword. However, there are some rules regarding which types can be cast to and from other types.

### Invalid Cast Types

`string`, `array`, `ptr`, and `struct` types cannot be cast to or from any type, though individual elements of an `array` and individual `struct` members can be, provided they are a valid casting type.

For example, the following cast is valid:

    alloc array<3, int> my_arr: { 0, 1, 2 };
    alloc float my_float: my_arr[2] as float;

But saying something like this is not:

    alloc string my_string: "123";
    alloc int my_int: my_string as int;

Strings must be fully parsed in order to be converted to another type, which is not provided by the `as` keyword. Instead, a function like the standard library `itos` or `stoi` must be used.

### Valid Cast Types

Any numeric type may be converted to or from any other numeric type. Integers become floats with a fractional portion of 0, and floating-point numbers simply have their fractional portion removed. Such functionality is even included in the x86 instruction set's SSE extensions (like `cvtsi2ss` or `cvttss2si` to convert a doubelword integer to and from a single-precision floating-point number, respectively). An example:

    alloc int x: 30;
    alloc float f: x as float;  // f = 30.0
    alloc float pi: 3.14159;
    alloc int n: pi as int; // n = 3

Conversions to bool from numerics will yield `false` if equal to 0 (or 0.0), and `true` for any non-zero number. Similarly, a value of `false` will yield a value of 0 and `true` will yield 1 (or 0.0 and 1.0 for floating-point numbers, respectively).

The following is a matrix of type conversions allowed using `as`:

| Type | Cast to `bool` | to `int` | to `float` | to `string` | to `array< T >` |
| ---- | -------------- | -------- | ---------- | ----------- | --------------- |
| `bool` | | `false` is `0`, `true` is `1` | `false` is `0.0`, `true` is `1.0` | Expressed as `"false"` or `"true"` | |
| `int` | 0 is `false`, all non-zero are `true` | | Equivalent whole floating-point number | | Expressed as string | |
| `float` | 0 is `false`, all non-zero are `true` | Remove fractional portion | Expressed as string | |
| `string` | Empty strings are `false`, non-empty strings are `true` (uses `string:len as bool`) | Must use standard library string parsing | Must use standard library string parsing | | Casts to `array<str:len, char>` |

You may also specify widths and signs, and the compiler may issue a warning about potential data loss.

### Sign and Width Conversions

Casts may also be done to change or specify the width or sign of a type, though decreasing a type's width (or range of values) might result in data loss. This will not generate a warning, as it is standard behavior. For example, mixing `unsigned int` and `signed int` will typically generate compiler warnings, but if typecasting is used, these warnings will go away. For example:

    alloc int x: 31;
    alloc int y &unsigned: 415;
    
    let x = x & y;  // generates compiler warning about differing signs

    alloc unsigned int z: x & y as signed int;    // no warning generated here

If there is a sign change between the two types, nothing happens to the data; it is copied bit-for-bit between each type. For a 16-bit integral type, this means the following behavior exists:

    alloc short int x: -1;
    alloc unsigned short int y: x as unsigned short int;    // y is now 65535

Since -1 is `0xFFFF`, this data will simply be moved in to the new integer type. Because it is unsigned, it is interpreted as 65535, which is also `0xFFFF`. If this behavior is not desired, the `as` keyword should not be used. Since SIN types are fixed-width (like Rust, unlike C), the new representation is predictable. If the type is being moved to a larger one, it is always zero-extended on the unused bits from the original type.

### Literal Quality Overriding

In addition to the `as` keyword, it is also possible to perform typecasting on literal values by utilizing _quality overriding._ Instead of a typecast, a postfixed quality may be used. For example, the integer value `1_000` will normally be treated as a 32-bit `int` type, but we may say `1_000 &long` to force the compiler to treat it as a 64-bit `long int`.
