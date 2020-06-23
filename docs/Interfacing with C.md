# SIN Documentation

## Interfacing with C

To provide more functionality to SIN, and allow easier integration with existing frameworks, the language supports native interfacing with C programs. This may be done through the use of a few select keywords that indicate to the compiler that various variables, etc. are located elsewhere and should be resolved at link time. This is chiefly done with the `decl` keyword, but calling convention modifiers may be used with functions to indicate to the compiler that calls should follow a calling convention other than `sincall`.

This creates a few problems, chiefly among them type compatibility. Because widths in C are not fixed, but rather only specify the *minimum* width for a type, and considering C types are, at times, slightly different than SIN types (such as `struct`, `array`, and `string`), some conversions must be made by the compiler when this interfacing is done. As such, the compiler should always know to expect a C type rather than a SIN type. With functions, this is done automatically according to the table below.

_Note: currently, the goal is to make C callable from SIN, but the details of interaction the other way (in regards to types and safety) have not yet been solidified._

### Type Conversion

C types transform to SIN types according to the following table. Note that currently, not all C types are supported.

| C Type | SIN Type | Width | Notes |
| ------ | -------- | ----- | ----- |
| `int` | `int` | 32 bits | Signed by default in both languages |
| `short` | `short int` | 16 bits | |
| `long` | `long int` | 64 bits | |
| `char` | `char` | 8 bits | |
| `bool` | `bool` | 8 bits | C booleans may only be used with the proper C header file |
| Pointers | `ptr<T>` | 64 bits | A pointer to any type is transformed to a SIN `ptr` type appropriately |
| `char[]` and `const char*` | `string` | Variable | C strings are null-terminated, so the string's length must be calculated when importing a string from C. When calling a C function that takes a C-string as a parameter, the address of the first character in the SIN string will be supplied; SIN strings always contain a null byte (`\0`) at the end, so they will be properly formatted for C |
| Other array types | `array<T>` | Variable | C arrays will utilize `array` _except for_ `char[]`, which are transformed to `string`. Note that because C arrays do not store the length, it can be tricky to utilize them in SIN |

### Functions

When a user wishes to use an external C function, its calling convention *must* be specified; else, serious runtime errors will inevitably occur and stack corruption is all but guaranteed. Similarly, when a SIN function is to be exported for use in C (or any other language), its calling convention should be changed from the default `sincall` to something that is supported by the target compiler (such as `cdecl` or `stdcall`). Failure to do so will, again, likely result in runtime errors and stack corruption because the SIN code will expect return values and parameters to be in different locations than the C code.

### Arrays

C arrays, unlike SIN arrays, do not store the array length with the data. This is often a source of major security issues in C programs because they are vulnerable to [buffer overflow attacks](https://en.wikipedia.org/wiki/Buffer_overflow). When calling C functions from SIN, this doesn't cause an issue, as it skips the length doubleword, though the compiler will issue a note saying that such arrays are memory-unsafe.
