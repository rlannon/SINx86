# SIN Documentation

## Procedure Objects

_Note: This document describes a future addition to the SIN language, one which is not currently present or in development within the compiler. As such, some of the details of its behavior may be adjusted._

One of the quirks about SIN syntax is its function calls; why use the `@` before the function name? Why not do what C, Java, Python, Rust, etc. do and just say `name(arguments)`?

This was one of the first planned features of the language and one which was inspired by assembly programming and function pointers. In most C-like languages, using the name of a function references it, and actually placing arguments in parentheses is what calls it. But if you wish to store a function pointer *and* the arguments you want to supply, you need to use wrappers, and it can be kind of a mess. Yes, it's possible, but it's something I wanted to be *easy* in SIN. And that's where the idea of a procedure object came in--a type that stores the routine's address and the arguments you wish to supply it when that object is invoked. In SIN, that describes the `proc` type.

But that doesn't answer the question of why the `@` is needed. The idea is that in SIN, the `@` operator--or the control transfer operator--is used to transfer program control to somewhere else. This can be done with any defined function or with a `proc` type. The idea for this operator came from assembly, where you have instructions like `jsr my_function` (MOS 6502) or `call my_function` (x86). This `jsr` or `call` instruction has a direct analogue -- the control transfer operator. This operator has a slightly lower precedence than the dot operator in order to allow for a future language extension (which will add `struct` member functions).

### Basic usage

The basic `proc` type is

    proc< R, tuple< A1 .. A(n) > >

Where `R` is the function's return type and the tuple contains the types of its parameters (labeled `A1` through `A(n)`), in order. An example of how these are used:

    alloc proc p< void, tuple< string > >;
    let p = print("Hello, world!");
    @p;

Here, a procedure object is created which expects a function with a return type of `void` and a single `string` parameter. This could be *any* function that satisfies those requirements, and `p` could be assigned an arbitrary number of times. In this case, it is assigned to point to the `print` function and contain a parameter `"Hello, world!"`. To call the function contained by `p`, just use the control transfer operator `@`.

We could also, if we desire, call it with our own parameters and ignore the default ones supplied:

    @p("Goodbye, world!");

See below for more information on this and parameter rules with `proc`.

### Use as an anonymous function

Another use of the `proc` type is to create anonymous functions and lambda expressions. To do this, the `function` keyword and the operator `=>` must be used (similar to JavaScript's anonymous functions). An example of how this is done:

    alloc proc multiply< int, tuple< int, int > >;
    let multiply = function(x, y) => {
        return x * y;
    };

We can then call it like so:

    @multiply(2, 3);

In the above example, we do not need to supply type information with `function` because the signature must exactly match with the signature of the `proc` object. However, if we wish to supply type annotations, we may (and generally should -- it is good practice):

    int function(int x, int y)

Note that this is entirely optional -- the compiler will not generate any errors if type annotations are not supplied. It may generate a note to the programmer stating they are recommended. Note that although they are for the programmer's benefit, tf the type annotations do not match the type in the procedure, an error will be generated. Another example of this would be:

    alloc proc p< int, tuple< int, long float, final string > >;
    let p = function(i, f, s) => {
        ...
    };

Here, the compiler will match each named variable to its corresponding type information in `p`; `i` is an `int`, `f` is a `long float`, and `s` is a `final string`, and the procedure must return an `int`. If we provided type annotations like this:

    let p = int function(int i, float f, final string s) => { ... };

In this case, we will get an error because the type annotation for `f` does not exactly match the expected type -- we were supplied `float` when we really expected `long float`.

The procedure here may also be passed to a `proc` object as if it were any other function:

    alloc proc p< int, tuple< int, int > >;
    let p = multiply(10, 20);

Further, since `multiply` is just a `proc` object, it may also be reassigned to any other function. Note this will make the original function inaccessible in the process.

### How it gets compiled

Consider the following procedure:

    alloc proc p< void, (string) >: print("Hello, world!");
    @p;

We can see that when `p` is allocated, the compiler will reserve 2 quadwords on the stack; one for the pointer to `print`, the other for the pointer to a string (due to the nature of the string type). When control is transferred, the address is moved into register `rbx` and the arguments in the tuple are moved into registers and pushed onto the stack according to the pointed-to function's calling convention. Then, the compiler uses `call rbx` to transfer control. Otherwise, control flow is handled normally.

If the `proc` object's return type is `void`, it may be called as a statement, as with any other function; otherwise, it is treated as an expression and must be used within a statement, just like all other expressions in SIN.

#### Contained Parameters vs Arguments

The `proc` type may either have so-called _contained parameters,_ meaning parameters that are contained by the object such that we may call the object without supplying any arguments, or _arguments,_ meaning it is called like any other function and the programmer supplies arguments.

Because `proc` objects may be called with or without supplied parameters, the object must _always_ contain a valid, constructed `tuple`. If arguments are supplied when the object is called, these contained parameters are ignored; else, the object's contained paramters will be utilized.

These contained values may be initialized with non-const values, though they will be considered like any other variable -- changes in the initial value will not be retained unless a pointer type is used. Consider the following:

    alloc int x: 30;
    alloc proc< int, tuple< int, int > > p;
    // calling p here would be illegal as it hasn't been initialized
    let p = multiply(x, 2);
    let x = 50;
    @p; // calls multiply(30, 2)

However, we can utilize pointers to pass in some value if we want to retain changes to that value, as is the case with any variable.

When we initialize a `proc` object as an anonymous function, _this rule still applies_ because it is impossible to know for certain whether some function has contained parameters until runtime. As such, the tuple will initialize its contained values according to the rules of [type construction](Construction). For example:

    alloc proc< bool, tuple< int, int > > divides;
    let divides = function(dividend, divisor) => {
        return dividend % divisor = 0;
    }
    @divides;

This is perfectly legal SIN _despite the fact_ that no arguments were ever supplied to `divides`; `proc` objects are _always_ guaranteed to be callable without arguments because type construction rules apply. In this case, following the construction rules for `int`, the call to `divides` is effectively:

    @divides(0, 0);

because an `int` will always be default-constructed to zero.
