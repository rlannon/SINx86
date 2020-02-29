# SIN Documentation

## Hidden Pointer Types

SIN has three types--`string`, `array<T>`, and `struct`, that are so-called "hidden pointer types" because although they don't use pointers *syntactically*, under the hood, these types are implemented through the use of pointers to the data. Similarly, all data marked with the `dynamic` qualifier will use pointers to data in the heap even though pointers are not used in the syntax (but they *can* be, under certain circumstances). Here is an example:

    alloc string my_string: "hello, world!";
    alloc string second_string: my_string;

The `string` type cannot be passed in a register, but a *pointer* to a string type can; so, when a string type is evaluated, the address of that string is acquired in a register and a special function is called to handle the assignment. This would generate some code like:

    mov rcx, strc_1 ; move address of strc_1 into rcx
    mov rdx, [rbp - 8]  ; move the address of 'second_string' into rdx
    call string_assignment  ; call the string assignment function

Under the hood, all such types are considered to be `ptr<T>` when used in expressions, and the dereferencing and data copies occur **automatically**.

Another example of the difference would be here:

    alloc int auto_int: 10;
    alloc int dyna_int: 20 &dynamic;
    alloc int third_int;

    let third_int = auto_int;
    let third_int = dyna_int;

The assignment statements will generate the following code (or something like it):

    ; assignment of automatic to automatic
    mov rax, [rbp]
    mov [rbp - 8], rax

    ; assignment of dynamic to automatic
    mov rsi, [rbp - 4]
    mov rax, [rsi]
    mov [rbp - 8], rax

### Type Overview

The following is an overview of these 'hidden pointer types.' Note that `array` and `string` may not be used as `sizeof< T >` arguments, but any user-defined `struct` type may, as a struct's width *must* be known at compile time.

**NB:** In subtypes, `T` is used to mean "type" (referring to any fully-parsed type) and `N` is used to indicate an integer.

#### `array< (N,) T >`

Arrays, like pointers, always contain a fully-parsed subtype; this tells the compiler what type of data is stored within the array, allowing it to be safely used in any expression as well as allowing the compiler to accurately calculate the array's width. Arrays may also contain, before the type, an unsigned integer indicating the number of elements in the array; this is *almost always* required, the only situations it is *not* being when:

* the array is a subtype of `ptr`; if a length is given, it will be ignored by the compiler (the programmer shall be notified this is the behavior by the compiler in a compiler note)
* the array is marked as `dynamic`; a length indicates how much initial memory should be reserved for the array, (possibly) preventing some of the overhead associated with reallocations

All arrays contain a 4-byte header containing an `unsigned int` indicating the number of elements contained by the array; this allows for runtime bounds and length checks without any additional variables to be tracked by the programmer.

**NB:** Arrays may not contain other arrays, but they may contain pointers to them; these pointer and array types must all be fully-parsed subtypes.

#### `string`

While strings are similar to `array<char>`, they are *not* identical in their behavior; `string` is a fully-fledged type while `array<char>` is a simple aggregate with markedly different behavior. The key difference is that automatic string types are still variable-length, while the same cannot be said of arrays. Arrays, unless marked as `dynamic`, are always of a fixed width which is known at compile time. Strings, on the other hand, are not, and always use dynamic (or `const`) memory. Although they may look identical in memory (4-byte length followed by characters), their behavior is different.

Strings may be marked as `dynamic`, and although this does not change where the string is located in program memory, it *does* affect the lifetime of the object; typically, when an automatic string goes out of scope, the compiler automatically `free`s it, invalidating any pointers to that string. However, when a string is marked as `dynamic`, the compiler will *not* free it automatically, allowing a pointer to it to be passed out of scope.

*A note about why* `string` *is a unique type:* The decision to add `string` to the language was done because of its ubiquity; since SIN's goal is to make the life of a programmer easier by reducing the use of confusing syntax and reducing manual memory management and pointer usage where possible, it makes more sense to have the `string` type rather than requiring programmers to create a `dynamic array<char>` and deal with the nightmares one might face in C (rather than, say, C++ or Python) every time they wish to utilize strings. While this may make the implementation of strings in the compiler a little more thorny, it reduces unnecessary difficulty when programming and so was deemed to belong in the language. Plus, it allows use of the concatenation operator (`+`) where `dynamic array<char>` would not.

#### `struct`

All user-defined struct types require their width to be known at compile time. This allows `array< T >` and `string` members in the same way any other scope would. When a struct includes these types, however, a few things must be kept in mind:

* if a struct has members which are marked as `dynamic`, they will not be automatically freed when the struct goes out of scope (they must be freed manually);
* all members will be freed if `free` is invoked on the struct **without exception;**
* if the user wishes to free specific members, invoking `free` on a specific struct member is allowed, and `free` may be called on the entire struct later as `free` will ignore any memory that has already been freed;
* if a struct has hidden pointer members, they will be automatically `free`d when the struct goes out of scope
* returning a pointer to a struct member is invalid *unless* said member is `dynamic` and `free` is not called on the struct object

For example, the following code is valid:

    def struct my_struct { 
        alloc string name;
        alloc string type;
        alloc int x;
        alloc int y;
    }

    def my_struct my_func() {
        alloc my_struct s;
        let s.name = "my_struct";
        let s.type = "user-defined struct";
        let s.x = 30;
        let s.y = 40;

        alloc my_struct s2;
        let s2.name = "my_struct 2";
        let s2.type = "another user-defined struct";
        let s2.x = 50;
        let s2.y = 100;

        return s;
    }

    def int main() {
        alloc my_struct ex: @my_func();
        @print(s.name + " is a " + s.type + "!");
        return 0;
    }

Since `my_func` returns the struct `my_func::s`, it will not be freed by the compiler; instead, a pointer to it will be returned and the assignment to `main::ex` will copy all of the data contained within `my_func::s` to `main::ex`. This will result in the destruction of all local variables but the preservation of dynamic memory.

Notice that if we had returned, for example, a `string` instead of a `my_struct` object, then we would have simply copied the string onto the stack and freed the structs in their entirety upon leaving the scope.
