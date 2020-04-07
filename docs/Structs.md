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
