# SIN Documentation
## SIN Calling Convention

The SIN calling convention is modeled after the x64 calling conventions for Windows and Linux, specifically `_cdecl`. However, the calling conventions differ slightly. Note that in SIN, the calling convention is declared in the function definition, _not_ in the call itself, as the convention affects how function bodies are generated.

The SIN convention is a **caller clean-up** convention which requires the caller to set up the stack frame for the callee and unwind it at the end. Unlike `_cdecl`, however, arguments are always pushed left-to-right, not right-to-left. Integral and pointer types will be pushed in registers `RSI, RDI, RCX, RDX, R8, R9`, while floating-point types will be pushed in registers `XMM0 - XMM5`. `RAX` and `RBX` are never preserved by the caller nor the callee; they are considered volatile.

Typically, this will end up looking something like:

    caller:
        ; function signature 'decl int callee(decl int a, decl int b, decl int c, decl int d, decl int e)'

        pushq rbp   ; preserve old call frame
        mov rbp, rsp    ; the new base is the current stack pointer

        ; pass arguments (call is '@callee(10, 20, 30, 40, 50)' )
        mov esi, 10
        mov edi, 20
        mov rcx, 30
        mov rdx, 40  ; first four integers passed in registers

        call callee ; call the function

        ; returned value is in EAX because the function returns a 32-bit integer

        mov rsp, rbp    ; restore the old stack frame
        popq rsp

        ; move the returned value into some variable from the higher scope
        mov [rbp - 16], eax

Now, we will look more in-depth at the rules for calling functions and returning values, as how values are passed and return depends on their data type.

### Function Arguments

#### Primitive Types
Primitive types (`float`, `int`, `char`, `bool`, `ptr`) will pass the first 6 parameters in registers if possible, as follows, and pass any subsequent data on the stack.

|   Type    |   Registers   |   Notes   |
| --------- | ------------- | --------- |
| `float` | XMM0 - XMM5 | How much of the register is used depends on whether it is single- or double-precision |
| `int` | RSI, RDI, RCX, RDX, R8, R9 | May use a different register width depending on type qualifiers |
| `bool` | SIL, DIL, CL, DL, R8B, R9B | Booleans will use a whole byte; they are not packed in this convention |
| `ptr` | RSI, RDI, RCX, RDX, R8, R9 | String values are passed as pointers and use the same registers |

Unlike the Windows x64 ABI, the SIN convention will pass more than six parameters in registers if it can. However, once an argument is passed on the stack, *all subsequent arguments will also be passed on the stack.* As a result, the maximum number of arguments that can be passed in registers is 12.

For example, the function

    decl void my_func(decl int a, decl ptr<int> b, decl float c, decl double d, decl int e &long, decl bool f, decl short int g, decl double h)

will pass values as follows:

| Variable | Register |
| -------- | -------- |
| `a` | ESI |
| `b` | RDI |
| `c` | XMM0 |
| `d` | XMM1 |
| `e` | RCX |
| `f` | DL |
| `g` | R8W |
| `h` | XMM2 |

Note that the compiler will allocate 'shadow space' for the register parameters on the stack and will make note of their offsets from the stack frame base, moving the stack pointer appropriately. This reflects the idea that all function parameters occupy space within the function as local variables, and are the first to be allocated in a function.

#### Aggregate and User-Defined Types
Aggregate types (arrays) and user-defined types (structs) must always be passed either as pointers in registers or on the stack. If the width is known at compile-time, they may be passed on the stack; otherwise, the caller will allocate memory for the object, copy the data into the newly-allocated area, and pass a pointer into the function. Note that syntactically, this is value does not appear to be passed as a pointer.

### Return Values
Values are returned on RAX (or another variant of the register depending on the data width) where possible. Floating-point types are returned in XMM0. Any unused bits in the register are undefined.

User-defined types (structs), arrays, and strings (if necessary) return a *pointer* to the data in `RAX`. This follows the convention for so-called 'hidden pointer types' where pointers are passed and assignments use copies and pointer dereferencing under the hood.

### Register Preservation
The only register preserved by this convention is `rbp`. All other registers must be preserved before the call if they need to be saved.

## Interfacing with C
The SIN calling convention also allows compilers to interface with C functions, and as such, there must be a way to ensure the SIN compiler handles arguments and return values properly. As such, a few keywords exist to alert the compiler to how a function should be called in the function declaration. Note these keywords may also be used with SIN functions, but must be done in the definition (and declaration, if present).

Such a declaration may look like:

    decl int myInt(decl int a, decl int b) &cdecl;

The SIN compiler will then know to generate code for calls to this function in accordance with the specified `_cdecl` calling convention.

In general these qualifiers default to the GCC ABIs used by Unix-like systems, but the `&windows` qualifier may be used to specify the user wishes to use the Microsoft convention instead. If you are worried the conventions may not be correct for your system, it is always a good idea to double-check your distribution to ensure these SIN features are compatible with your system.

### Calling Conventions

#### ```cdecl```
If a function declaration/definition uses the `cdecl` qualifier, it alerts the compiler that the function will use the C `_cdecl` calling convention.

#### ```cx64```
If a function uses the `cx64` qualifier, the function should follow the System V AMD x64 ABI (used by Unix-like systems).

### C Types in SIN
While calling conventions may be sorted out with out *too* much hassle, the difference between types and their sizes between the two languages proves to be a bit of a challenge. SIN will always assume the types for the C function's arguments and return values are the same widths as SIN functions.

Note that currently, SIN may not interface with C structs.
