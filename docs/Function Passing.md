# SIN Documentation

## Function Passing

While control may be transfered to subroutines in SIN with the `@` operator (or the `call` keyword if the subroutine call is not an expression), SIN also allows control to be passed completely to a new subroutine. For example:

    def int A(alloc int in)
    {
        return in * 2;
    }

    def int B(alloc int in)
    {
        pass->B(in + 1);
    }

    def int main(alloc dynamic array<string> args)
    {
        alloc int x: @B(10);
        return 0;
    }

In this case, `main` calls `B`, which _passes_ control to `A`; when `A` returns, control is returned directly to `main`. When the compiler encounters the `pass` statement in `B`, it creates `A`'s stack frame in place where `B`'s was and _jumps_ to `A` rather than utilizing an x86 `call` instruction. Thus, when `ret` is reached, it pops the address in `main` off the stack.

Note that when `pass` is used, the passed-to function _must_ have the same return type as the function in which the `pass` occurs. This is because the passing function does not return a value; it passes control to a new function which will return the value instead.

### Purpose

This feature was added in order to allow future thread-based programming with X-SIN. The idea behind `pass->f()` (or the more compact `@->f()`) is that stack usage is minimized, allowing delegating or startup functions to be destroyed when they pass control. For example, in a model where a `boot` function is used, one whose sole purpose is to set up the environment and relinquish control, the `boot` function will cease to exist entirely when it passes control on to the next thread.

### Rules

1. A passed-to subroutine must have the same return type as the passing function.
2. A `pass` statement may not be used in combination with any type of expression; it may _only_ contain a function signature.
3. `pass` may utilize a keyword with the move operator _or_ the control transfer operator with the move operator.

The compiler is also allowed (but not required) to optimize some `return` statements by transforming them into `pass` statements. When a function encounters a return statement whose return expression is a `call` expression, it may utilize control passing instead (in order to preserve stack space). However, if the value is a part of an expression (such as a binary or a typecast), it must utilize a regular return statement.
