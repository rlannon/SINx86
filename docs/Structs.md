# SIN Documentation

## Structs

SIN, like many programming languages, allows users to define their own types in order to expand the language's functionality. This may be done via structs, which are more or less equivalent to their C counterparts. Once defined, they may be allocated like any other data type. Although SIN is not object-oriented (and does not support inheritance or polymorphism), there is a benefit to allowing operator overloading for user-defined types as well as member functions.

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
    ]   = rpb - [26 - 10]
        = rbp - 16

and sure enough, looking at the sample memory structure, `m.x` is located at `rbp - 16`.

### Static Members

Structs may contain static members, meaning they don't need to be accessed from any one particular object. If a specific object is referenced, they may use the dot operator as normal, like `b.c`. However, if the static member is accessed without a particular object reference, it may use the attribute operator -- e.g., `a:c`.

## Construction

_Note: this section describes a future addition to the language, one which is not currently present or in development within the compiler._

An important aspect of structures is that they are constructed when they are initialized, similar to languages like Rust or C++. The syntax for construction is similar to that of Rust, except that the `construct` keyword is used to specify construction. For example, assuming we have some struct `point`:

    alloc point p: construct point {
        x: 0,
        y: 0,
        z: 0,
    };

_(Note that the `point` in the construct expression is not strictly necessary when using alloc-init syntax, as the type is already known)_

However, this keyword may be used in four distinct scenarios:

* Initialization with alloc-init syntax
* New struct creation (replacement of alloc-init syntax)
* Whole struct assignment
* Struct replacement via anonymous struct creation

The above example used alloc-init syntax. If a struct contains references, this will still work because a new struct is created that replaces the old one. For example, this code is valid:

    alloc int x: 10;
    alloc int y: 10;
    def struct m {
        alloc ref<int> r;
    }

    // alloc-init intialization
    alloc m m1: construct {
        r: $x,
    };

    // new struct creation without 'alloc'
    construct point p1 {
        x: 10,
        y: 20,
        z: 30,
    };

    // struct assignment
    alloc point origin;
    construct origin {
        x: 0,
        y: 0,
        z: 0,
    }

    // struct replacement with an anonymous struct
    let m1 = construct point {
        r: $y,
    };

Typically, references are immutable, but in this instance, the entire struct `m1` is deleted and replaced with a new object. This means that we aren't really altering its references, we are replacing them.
