# SIN Documentation

## Structs

SIN, like many programming languages, allows users to define their own types in order to expand the language's functionality. This may be done via structs, which are more or less equivalent to their C counterparts. Once defined, they may be allocated like any other data type. However, because SIN does not support operator overloading or member functions (as is the case in C), structs may only be directly assigned to with other structs of the same type, and any initialization functions must be written as regular functions, not methods. For example:

    alloc some_struct s;
    let s = 10; // illegal; '10' is not a struct type

    alloc some_struct t;
    @my_initialization($t, 10);  // not a method
    let s = t;  // valid; s and t are both the same type

### Defining Structs

Structs, like functions, are *defined* by the user so that the compiler knows how to treat references to them. The keyword *def* is used, followed by the keyword *struct*; the compiler knows this is not a function definition because *struct* is not a type, but a keyword to alert the compiler that a new type is being defined.

Within the definition of a struct, only allocations are allowed, though alloc-init syntax is allowed so that the compiler knows to give data members default values. When `alloc` is used on a struct type, allocations of all members are performed; when `free` is called on a struct type, all data members are freed (though members may also be freed individually). For example:

    def struct point
    {
        alloc int x;
        alloc int y;
    }

    alloc point p;  // allocates an object 'point' by allocating its members, x and y, in that order

SIN does not require that structs be padded and will not reorder elements, unlike compilers for some other languages.

Note, also, that a struct may not contain an instance of itself, as this would cause infinite recursion. Instead, one must use a pointer if such behavior is desired.

### Struct Member Order

Structs are *always* organized such that the first declared struct member is located at the lowest memory address. This means that when allocated on the stack, their members will be pushed onto the stack in *reverse* order. This was done for consistency reasons and ease of returning struct and array objects on the stack through pointers, as well as for ease of copying struct and array objects between the stack and other areas of memory. For example, the following code:

    def struct my_struct {
        alloc short int a;
        alloc int b;
        alloc float x;
        alloc double y;
    }

    def void main() {
        alloc my_struct m;
        alloc int c;
    }

will look like this on the stack when allocated:

    rbp - 8:    m.y
    rbp - 16:   m.x
    rbp - 20:   m.b
    rbp - 24:   m.a
    rbp - 26:   c

The formula to calculate where a given struct member is located on the stack is:

    rbp - [
        (struct's stack offset + struct's total width) -
        (struct member offset + member's width)
    ]

So, that means that the offset of `m.x` from the example above is:

    rbp - [
        (8 + 18) -
        (6 + 4)
    ] = rpb - [26 - 10] = rbp - 16

and sure enough, looking at the sample memory structure, `m.x` is located at `rbp - 16`.
