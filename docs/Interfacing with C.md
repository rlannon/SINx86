# SIN Documentation

## Interfacing with C

To provide more functionality to SIN, and allow easier integration with existing frameworks, the language supports native interfacing with C programs. This may be done through the use of a few select keywords that indicate to the compiler that various variables, etc. are located elsewhere and should be resolved at link time. This is chiefly done with the `decl` keyword, but calling convention modifiers may be used with functions to indicate to the compiler that calls should follow a calling convention other than `sincall`.

This creates a few problems, chiefly among them type compatibility. Because widths in C are not fixed, but rather only specify the *minimum* width for a type, and considering C types are, at times, slightly different than SIN types (such as `struct`, `array`, and `string`), some conversions must be made by the compiler when this interfacing is done. As such, the compiler should always know to expect a C type rather than a SIN type. With functions, this is done automatically according to the table below.

### Type Conversion

C types transform to SIN types according to the following table. Note that currently, not all C types are supported.

| C Type | SIN Type | Width | Notes |
| `int` | `int` | 32 bits | Signed by default in both languages |
| `short` | `short int` | 16 bits | |
| `long` | `long int` | 64 bits | |
| `char` | `char` | 8 bits | |
| `bool` | `bool` | 8 bits | C booleans may only be used with the proper C header file |
| Pointers | `ptr<T>` | 64 bits | A pointer to any type is transformed to a SIN `ptr` type appropriately |
| `char[]` and `const char*` | `string` | Variable | C strings are null-terminated, so the string's length must be calculated when importing a string from C. When calling a C function that takes a `string` as a parameter, the address of the first character in the SIN string will be supplied |
| Other array types | `array<T>` | Variable | C arrays will utilize `array` except for `char[]`, which is transformed to `string`. Note that C arrays do not store the length, which can make it tricky to utilize them in SIN |

### Functions

When a user wishes to use an external C function, its calling convention *must* be specified; else, serious runtime errors will inevitably occur and stack corruption is all but guaranteed. Similarly, when a SIN function is to be exported for use in C (or any other language), its calling convention should be changed from the default `sincall` to something that is supported by the target compiler (such as `cdecl` or `stdcall`). Failure to do so will, again, likely result in runtime errors and stack corruption.
