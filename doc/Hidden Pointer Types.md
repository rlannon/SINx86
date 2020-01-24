# SIN Documentation
## Hidden Pointer Types

SIN has three types--`string`, `array<T>`, and `struct`, that are so-called "hidden pointer types" because although they don't use pointers *syntactically*, under the hood, these types are implemented through the use of pointers to the data. Similarly, all data marked with the `dynamic` qualifier will use pointers to data in the heap even though pointers are not used in the syntax (but they *can* be, under certain circumstances). Here is an example:

    alloc string my_string: "hello, world!";
    alloc string second_string: my_string;

The `string` type cannot be passed in a register, but a *pointer* to a string type can; so, when a string type is evaluated, the address of that string is acquired in a register and a special function is called to handle the assignment. This would generate some code like:

    mov rcx, strc_1 ; move address of strc_1 into rcx
    mov rdx, [rbp - 8]  ; move the address of 'second_string' into rdx
    call string_assignment  ; call the string assignment function

Under the hood, all such types are considered to be `ptr<T>`.

Another example of the difference would be here:

    alloc int auto_int: 10;
    alloc int dyna_int: 20 &dynamic;
    alloc int third_int;

    let third_int = auto_int;
    let third_int = dyna_int;

The assignment statements will generate the following code:

    ; assignment of automatic to automatic
    mov rax, [rbp]
    mov [rbp - 8], rax

    ; assignment of dynamic to automatic
    mov rsi, [rbp - 4]
    mov rax, [rsi]
    mov [rbp - 8], rax
