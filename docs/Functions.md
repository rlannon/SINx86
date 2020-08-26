# SIN Documentation

## Functions

Functions are an important part of any imperative programming language, and SIN is no exception. SIN functions are nearly identical to those of other C-style languages, as they may have any number of parameters and may return a value of a specified type. Functions allow for modularization and are crucial tools for essentially all programming tasks.

### Syntax

Functions in SIN may be defined using the `def` keyword alongside the return type and function name, followed by parameters in parentheses; these parameters are local variables and so are allocated like any other variable. For example:

    def int my_func(alloc int a, alloc int b) {
        return a + b;
    }

Functions may also be declared, like external static memory, using the `decl` keyword; such declarations also require parameters to be declared with the `decl` keyword. For example:

    decl int my_func(decl int a, decl int b);

This separates the function's declaration and definition, giving more freedom to the programmer and allowing precompiled libraries to be used with header files.

### Rules

Functions have a few rules to keep in mind, and the aim of this section is to explain them.

#### Parameters/Arguments

First and foremost, a function's *parameters* are the variables expected by the function call; in the first example, the parameters were `int a` and `int b`. The function's *arguments* are the values which are actually passed to the function in a call. For example, in `@my_func(10, 20)`, `10` and `20` are the arguments. Sometimes, parameters and arguments are called formal and actual parameters, respectively. In SIN, either naming system is acceptable, but "parameters" alone will always refer to formal parameters.

All function parameters are local variables that are initialized in the function call. Importantly, *all* parameters must be initialized in the call to any given function. That said, these variables, like in C or Python, may have default values. Like C, once a default value is given to a parameter, all subsequent parameters must also take default values. Further, in a call, arguments are assigned to their respective parameters based on the argument's position -- named arguments are not allowed in SIN. For example:

    def int my_func(alloc int a, alloc int b: 10, alloc int c: 20){
        return a + b + c;
    }

Calling the function like `@my_func(1, 2)` will assign `a: 1`, `b: 2`, and `c: 20`.

Parameters may have `static`, `dynamic`, `final`, or `const` qualities, though caution must be exercised in some cases:

* a `static` parameter will, of course, not be allocated on the stack;
* a `dynamic` parameter will perform a data copy to dynamic memory, passing by value; it will never serve as a pass by reference (if this is desired, use a `ptr< T >`);
* `const` qualities are completely useless (but not strictly illegal) as they must have a default value and cannot take a value in the call; if a function signature contains a `const` parameter, a compiler warning will be generated saying `final` may be what the programmer intended, as `const` values are compile-time constants while `final` data are run-time constants

#### `return` values

All functions must have a `return` statement in *every* control path. Even `void` functions are required to have the statement `return void`. A lack of a `return` statement on any control path in a function will generate a compiler error; a programmer is not allowed to skirt the rules of value-returning functions through control flow structures.

Functions may return data of any type, though the following qualities on return types are either illegal or discouraged (generating errors or warnings, respectively):

| Quality | Legality | Reason |
| ------- | -------- | ------ |
| `static` | Illegal | Return values are always temporary values returned in a register or on the stack, and as such, a storage specifier on a return type does not make sense |
| `dynamic` | Illegal | Same reason as for `static` |
| `final` | Legal | The keyword does little in this context due to the nature of return values being temporary, computed values. However, this specifier is important when used with pointers and references. |
| `const` | Illegal | Subroutines may have side effects that make it impossible to compute them at compile time, which is what the `const` keyword specifies |

All width- and sign- modifying qualities are allowed in accordance with normal type rules.
