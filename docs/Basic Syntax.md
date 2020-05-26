# SIN Documentation

## Basic Syntax

Being a C-style language, and being heavily inspired by C++ and Python, the syntax of SIN should be pretty familiar to anyone who has used these languages in the past. SIN takes various elements from each, sometimes repurposing them; while this may, in some cases, cause confusion, the language syntax is designed to be very clear to the reader and easy to remember for the programmer.

Like other C-style languages, and being an imperative language, a SIN program is comprised of a series of *statements*, each containing any number of *expressions*.

## Statements

Statements in SIN must begin with either a [statement keyword](Language%20Keywords.md) or the `@` operator to call a `void`-returning function. A value-returning function is considered an expression, and so it is illegal for it to stand alone.

### Scope Blocks

One type of statement that is utilized in function definitions, while loops, and if-else statements is the scope block. Like C, such blocks use curly braces (`{` and `}`) to indicate some code that is grouped together and allows for variables local to it. Take the following examples -- they generate equivalent source code:

    if (x = 10)
    {
        let y = x;
    }
    else {
        let y = 0;
    }

and

    if (x = 10)
        let y = x;
    else
        let y = 0;

Although they generate the same code, they will be interpreted by the parser a little bit differently. In the first example, we can always add more lines of code to the branches later without creating any issues; if we do so in the second, like:

    if (x = 10)
        let y = x;
        let x = 30;     // not part of the if block
    else                // stand-alone 'else', as parsing 'if' was completed
        let y = 0;
        let x = 0;      // not part of the else block

We will ultimately have two errors, one syntactic and one logical. The syntax error is that, since we don't use braces, the statement `let x = 30` will always be executed, as it is not part of the if-else block, meaning once the parser hits the `else` statement, there is a stray keyword in the program (and `else` is not a valid beginning to a statement). The logical error is that, barring the syntactic error, the statements `let x = 30` and `let x = 0` will *always* execute in this program because they are not a part of the branch block; the branches may only be one statement each. To get around this, curly braces may be used to create a single "statement" that consists of a chunk of code.

## Expressions

Like other C-style languages, statements in SIN rely on various types of *expressions.* The available expression types are:

* Value-returning function calls
* Unary expressions
* Binary expressions
* Symbols
* Literals

## Operators and Precedence

### Operators

Operator precedence in SIN closely follows operator precedence in C and Python -- and, actually, those languages were used to determine SIN operator precedence.

The following operators are defined in SIN (in descending order of precedence):

| Precedence | Operator | Associativity | Defined Types | Purpose |
| ---------- | -------- | ------------- | ------------- | ------- |
| 25 | Dot (`.`) | Left-to-Right | `struct` | Member selection on `struct` types |
| 25 | Attribute (`:`) | Left-to-Right | Any | Attribute selection |
| 24 | Unary minus (`-`) | Right-to-Left | Any numeric | Opposite (negative) of the given rvalue |
| 24 | Unary plus (`+`) | Right-to-Left | Any numeric | Does nothing, but available to contrast the unary minus |
