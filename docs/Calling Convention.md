# SIN Documentation

## SIN Calling Convention

The SIN calling convention is modeled after the x64 calling conventions for Windows and Linux, but the calling conventions differ in a few important ways that make them incompatible. However, the compiler can accomodate for these differences and allow different calling conventions to be used, provided the user supplies the appropriate information.

### General Overview

The SIN convention is a **caller clean-up** convention which requires the caller to set up the stack frame for the callee and unwind it at the end. Unlike `_cdecl`, however, arguments are always pushed left-to-right, not right-to-left. Integral and pointer types will be pushed in registers `RSI, RDI, RCX, RDX, R8, R9`, while floating-point types will be pushed in registers `XMM0 - XMM5`. `RAX` and `RBX` are never preserved by the caller nor the callee automatically; they are considered volatile. `YMM` and `ZMM` registers are currently not utilized by the language.

In the SINCALL convention, function arguments exist _above_ the stack frame, meaning arguments are written into memory before the new stack frame is set up. This allows for easier evaluation of their values when called. Generally, the following happens in SINCALL:

* First, the stack pointer is decremented to be below the function arguments; any arguments written to the stack will be written to a location like `[rsp + 8]`
* In turn, each argument:
  * is evaluated
  * is stored either in a register or in memory, depending on how it is to be passed
* The status register is pushed
* The base pointer is pushed
* The stack pointer is moved into the base pointer, setting up a new frame
* The function is called

The reverse is done when the function returns. Typically, a call in this convention will end up looking something like:

    caller:
        ; function signature 'int callee(int a, int b, int c, int d, int e &long)'

        sub rsp, 20 ; we need to reserve space for these as local variables above where we set up the frame

        ; pass arguments (call is '@callee(10, 20, 30, 40, 50)' )
        mov esi, 10
        mov edi, 20
        mov ecx, 30
        mov edx, 40
        mov r8, 50  ; we can pass all of these in registers

        pushfq  ; preserve the status
        push rbp   ; preserve old call frame
        mov rbp, rsp    ; the new base is the current stack pointer

        call callee ; call the function

        ; returned value is in EAX because the function returns a 32-bit integer

        mov rsp, rbp    ; restore the old stack frame
        pop rsp
        pop rflags  ; restore the flags
        add rsp, 20 ; move rsp back to where it was before the call

        ; move the returned value into some variable from the higher scope
        mov [rbp - 16], eax

Note, then, that the return address will be located somewhere in the stack below `ebp`, exactly where depending on the number (and type) of parameters. Parameters will still need space in the stack *above* the return address, no matter whether they were passed in registers or on the stack. So, our stack will look like:

|   Offset  |   Data    |
| --------- | --------- |
| +36 | `int a` |
| +32 | `int b` |
| +28 | `int c` |
| +24 | `int d` |
| +16 | `long int e` |
| +8 | `rflags` |
| +0 | `rbp` |
| -8 | return address |

This means that before the function returns, it has to be sure to move the stack pointer back to `rbp - 8`, as that is the location of the return address.

This calling convention therefore means that all parameters will always be located at a location _above_ `rbp`. Specifically, they will be located at:

    rbp + 16 + offset

to account for the width of the old base pointer (8 bytes), the width of `rflags` (8 bytes), and then the offset where it lies within the stack. This means the last parameter will have an offset of 16, while the first parameter will have an offset of 16 + the widths of all parameters that come after it.

### Stack Alignment

One of the key differences between SINCALL and the Windows or System V ABIs is that SINCALL never requires any particular stack alignment on a function call. Other x64 ABIs require a 16-byte alignment for SSE instructions, but in the SIN convention, it is the _callee,_ not the _caller,_ that is responsible for aligning the stack if it requires a 16-byte alignment.

### Function Arguments

#### Primitive Types

Primitive types (`float`, `int`, `char`, `bool`, `ptr`) will pass the first 6 parameters in registers if possible, as follows, and pass any subsequent data on the stack.

|   Type    |   Registers   |   Notes   |
| --------- | ------------- | --------- |
| `float` | XMM0 - XMM5 | How much of the register is used depends on whether it is single- or double-precision |
| `int` | RSI, RDI, RCX, RDX, R8, R9 | May use a different register width depending on type qualifiers |
| `bool` | SIL, DIL, CL, DL, R8B, R9B | Booleans will use a whole byte; they are not packed in this convention |
| `ptr`, `ref`, and `string` | RSI, RDI, RCX, RDX, R8, R9 | String values are passed as pointers and use the same registers |

Unlike the Windows x64 ABI, the SIN convention will pass more than six parameters in registers if it can (other conventions will pass at most 6, but SINCALL could theoretically pass up to 12 because it could pass a combination of integral and floating-point types). However, once an argument is passed on the stack, _all subsequent arguments will also be passed on the stack._ As a result, the maximum number of arguments that can be passed in registers is 12.

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

Note that the compiler will allocate 'shadow space' for the register parameters on the stack and will make note of their offsets relative to the stack frame base. This reflects the idea that all function parameters occupy space within the function as local variables, and are the first to be allocated in a function.

#### Aggregate and User-Defined Types

Aggregate types (arrays, tuples) and user-defined types (structs) must always be passed either as pointers in registers (if passed as a pointer or reference, such as using `ptr<T>` or `dynamic T`) or on the stack, if the width is known at compile time (meaning the type is not dynamic).

### Return Values

Values are returned in `rax` (or another variant of the register depending on the data width) where possible. Floating-point types are returned in XMM0 as scalar values. Any unused bits in the register are undefined; as an example, returning a value in `al` does not necessarily mean that `ah` will have been zeroed.

#### Non-Primitive Return Values

If a function returns any value that cannot fit into a register _by value,_ such as a struct, it is returned via the stack:

* A pointer is passed to the function to indicate where this data is to be placed, as data cannot live below `RSP`.
* This pointer could refer to a temporary variable, but it could also point to an existing stack-allocated variable from the caller. This allows for certain return-value optimizations.
* Sometimes, a temporary variable is necessary; this is determined by the caller.
* The returned value will be copied into the location indicated by the return pointer, and this pointer will be in `RAX` when the function returns.
* This pointer will always be the first 'parameter' to the function, and will be in the stack before `this` in a member function. However, when passed in a register, it will be passed with `R9`.

Remember that structs and arrays are written in "reverse order" onto the stack (first byte at low address, last byte at high address). This allows us to easily copy between memory areas without needing to worry about reversing the way we write data. The formula for figuring out where a given struct member is in the stack is:

    (struct_stack_offset + struct_width) - (struct_member_offset + struct_member_width)

Writing structs to the stack in reverse order will make it easy to copy between the stack and other areas of memory (as struct member order does not need to be accounted for).

### Register Preservation

The only registers that are always preserved by this convention are `rbp` and `rflags`. All other registers must be preserved before the call--specifically, before `rsp` is modified--if they need to be saved.

## Interfacing with C

For more information see [this document](Interfacing%20with%20C).

The SIN calling convention also allows compilers to interface with C functions, and as such, there must be a way to ensure the SIN compiler handles arguments and return values properly. As such, a few keywords exist to alert the compiler to how a function should be called in the function declaration. Note these keywords may also be used with SIN functions, but must be done in the definition (and declaration, if present).

Such a declaration may look like:

    decl int myInt(decl int a, decl int b) &c64;

The SIN compiler will then know to generate code for calls to this function in accordance with the appropriate C calling convention. Note that since this compiler targets x86-64 systems, it will only use 64-bit calling conventions as described [here](https://en.wikipedia.org/wiki/X86_calling_conventions#x86-64_calling_conventions).

In general these qualifiers default to the GCC ABIs used by Unix-like systems, but the `&windows` qualifier may be used to specify the user wishes to use the Microsoft convention instead. If you are worried the conventions may not be correct for your system, it is always a good idea to double-check your C compiler to ensure these SIN features are compatible with your system.

### Using Other Calling Conventions

SINCALL does not require a 16-byte stack alignment before a call, but other calling conventions do. As such, whenever a function that utilizes the calling convention for the Windows or System V ABIs, the compiler must generate code to properly align the stack on a 16-byte boundary. The [SRE](SIN%20Runtime%20Environment) handles this for certain built-in routines, and the compiler will handle it when calling non-SINCALL functions.

#### System V ABI (Unix-like systems)

If a function uses the `c64` qualifier by itself, the function should follow the System V AMD x64 ABI (used by Unix-like systems).

#### Windows

If a function uses the `windows` qualifier *in conjunction with* the `c64` qualifier, it will utilize the Windows 64-bit calling convention. Without the `c64` qualifier, use of `windows` will raise a compiler error.

#### SINCALL

By default, the `sincall` convention is used, though you may add it in for redundancy. If the `sincall` keyword is used, use of the other calling convention specifiers will generate an error.

### C Types in SIN

While calling conventions may be sorted out with out *too* much hassle, the difference between types and their sizes between the two languages proves to be a bit of a challenge. SIN will always assume the types for the C function's arguments and return values are the same widths as SIN functions.

Note that currently, SIN may not interface with C structs directly.
