# SIN Documentation

## SIN Calling Convention

The SIN calling convention is modeled after the x64 calling conventions for Windows and Linux, specifically `_cdecl`. However, the calling conventions differ slightly.

Note that in SIN, the calling convention is declared in the function definition, _not_ in the call itself, as the convention affects how function bodies are generated.

The SIN convention is a **caller clean-up** convention which requires the caller to set up the stack frame for the callee and unwind it at the end. Unlike `_cdecl`, however, arguments are always pushed left-to-right, not right-to-left. Integral and pointer types will be pushed in registers `RSI, RDI, RCX, RDX, R8, R9`, while floating-point types will be pushed in registers `XMM0 - XMM5`. `RAX` and `RBX` are never preserved by the caller nor the callee; they are considered volatile.

Typically, this will end up looking something like:

    caller:
        ; function signature 'decl int callee(decl int a, decl int b, decl int c, decl int d, decl int e &long)'

        push rflags ; preserve the status
        push rbp   ; preserve old call frame
        mov rbp, rsp    ; the new base is the current stack pointer

        ; pass arguments (call is '@callee(10, 20, 30, 40, 50)' )
        mov esi, 10
        mov edi, 20
        mov ecx, 30
        mov edx, 40
        mov r8, 50  ; we can pass all of these in registers

        sub rsp, 24 ; we need to reserve space for these as local variables above the return address

        call callee ; call the function

        ; returned value is in EAX because the function returns a 32-bit integer

        mov rsp, rbp    ; restore the old stack frame
        pop rsp
        pop rflags  ; restore the flags

        ; move the returned value into some variable from the higher scope
        mov [rbp - 16], eax

Note, then, that the return address will be located somewhere in the stack below `ebp`, exactly where depending on the number (and type) of parameters. Parameters will still need space in the stack *above* the return address, no matter whether they were passed in registers or on the stack. So, our stack will look like:

|   Offset  |   Data    |
| --------- | --------- |
| 0 | `rbp` |
| -4 | `a:int` |
| -8 | `b:int` |
| -12 | `c:int` |
| -16 | `d:int` |
| -24 | `e:long int` |
| -32 | return address |
| -40 | local `long int` |

This means that before the function returns, it has to be sure to move the stack pointer back to `rbp - 32`, which can be obtained simply by looking at the offset of the last parameter and adding the width of a pointer -- in this case, it will give us an offset of `24 + 8`, yielding a return location at`[rbp - 32]`

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

Values are returned on RAX (or another variant of the register depending on the data width) where possible. Floating-point types are returned in XMM0 as scalar values. Any unused bits in the register are undefined; as an example, returning a value in `al` does not necessarily mean that `ah` will have been zeroed.

#### Non-Primitive Return Values

User-defined types (structs), arrays, and strings (if necessary) return a *pointer* to the data in `RAX`. This follows the convention for so-called 'hidden pointer types' where pointers are passed and assignments use copies and pointer dereferencing under the hood.

Note that, since structs and arrays are written in "reverse order" onto the stack (first byte at low address, last byte at high address), we can easily copy between memory areas without needing to worry about reversing the way we write data. The formula for figuring out where a given struct member is in the stack is:

    (struct_stack_offset + struct_width) - (struct_member_offset + struct_member_width)

Writing structs to the stack in reverse order will make it easy to copy between the stack and other areas of memory (as struct member order does not need to be accounted for).

#### Secondary Return Values

Some SIN runtime library functions utilize multiple return values, meaning values will be contained in `rax` and `rbx`. While secondary return values are not a feature in SIN, the calling convention allows for them because they are required by the language. For example, the ASM dynamic memory allocation functions in the runtime environment return a pointer to the allocated memory in `rax`, but they will also return the number of allocated bytes in `rbx`.

### Register Preservation

The only registers that are always preserved by this convention are `rbp` and `rflags`. All other registers must be preserved before the call if they need to be saved.

## Interfacing with C

The SIN calling convention also allows compilers to interface with C functions, and as such, there must be a way to ensure the SIN compiler handles arguments and return values properly. As such, a few keywords exist to alert the compiler to how a function should be called in the function declaration. Note these keywords may also be used with SIN functions, but must be done in the definition (and declaration, if present).

Such a declaration may look like:

    decl int myInt(decl int a, decl int b) &c64;

The SIN compiler will then know to generate code for calls to this function in accordance with the appropriate C calling convention. Note that since this compiler targets x86-64 systems, it will only use 64-bit calling conventions as described [here](https://en.wikipedia.org/wiki/X86_calling_conventions#x86-64_calling_conventions).

In general these qualifiers default to the GCC ABIs used by Unix-like systems, but the `&windows` qualifier may be used to specify the user wishes to use the Microsoft convention instead. If you are worried the conventions may not be correct for your system, it is always a good idea to double-check your C compiler to ensure these SIN features are compatible with your system.

### Calling Conventions

#### System V ABI (Unix-like systems)

If a function uses the `c64` qualifier by itself, the function should follow the System V AMD x64 ABI (used by Unix-like systems).

#### Windows

If a function uses the `windows` qualifier *in conjunction with* the `c64` qualifier, it will utilize the Windows 64-bit calling convention.

#### SINCALL

By default, the `sincall` convention is used, though you may add it in for redundancy. If the `sincall` keyword is used, use of the other calling convention specifications will generate an error.

### C Types in SIN

While calling conventions may be sorted out with out *too* much hassle, the difference between types and their sizes between the two languages proves to be a bit of a challenge. SIN will always assume the types for the C function's arguments and return values are the same widths as SIN functions.

Note that currently, SIN may not interface with C structs directly.
