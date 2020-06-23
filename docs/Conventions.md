# Program Documentation

This document is to serve as a reference for conventions used in this project specifically, rather than the language in general. In particular, it aims to document the various naming and code generation decisions for the project and their rationale.

## Name Mangling

The SIN compiler utilizes minor name mangling in this implementation. While not strictly necessary _in theory_ in this version of SIN (as there is no function or operator overloading yet), because the compiler utilizes various C and C++ libraries that define the `_start` label, SIN utilizes the `main` label for runtime initialization, etc.. Further, it is in this function that any command-line arguments will be properly converted into a SIN `dynamic array`. As such, a SIN program cannot utilize the name `main` or the assembler would complain of a multiple definition.

As such, this compiler utilizes very minor name mangling, specifically the prefix `SIN_`. This is applied to all labels _except_ those that use the `extern` keyword. `extern` may not be used on `main`. Note this may change in the future, as the compiler may simply initialize the SRE in the user's `main` function and omit this type of name mangling. However, this method will remain in place if name mangling is eventually added to the language for real.

## Label Names

All `if .. else` and `while` statements, as well as things like string and floating-point constants (as x86 does not allow for immediate values with XMM registers), require the use of labels in order to properly keep track of memory locations and values. For this reason, the compiler maintains internal counts for string constants, floating-point constants, and label numbers. For example, this SIN code

    if (x % 2 == 0) {
        @print("even");
    }
    else {
        @print("odd");
    }

might generate something like the following x86:

    section .data
      SIN_STRC_0    .dw 4,`even`
      SIN_STRC_1    .dw 3,`odd`
    
    section .text
      xor edx, edx
      mov eax, [rbp - 12]
      mov ebx, 2
      cmp edx, 0
      jne SIN_ITE_ELSE_0
      lea rsi, SIN_STRC_0 ; if branch is here
      pushfq
      push rbp
      mov rbp, rsp
      call print
      mov rsp, rbp
      pop rbp
      popfq
      jmp SIN_ITE_DONE_0:
    SIN_ITE_ELSE_0:
      lea rsi, SIN_STRC_1 ; else branch
      pushfq
      push rbp
      mov rbp, rsp
      call print
      mov rsp, rbp
      pop rbp
      popfq
    SIN_ITE_DONE_0:
      ; continue execution  ; after 
