# SIN Documentation

## Basic Syntax

Being a C-style language, and being heavily inspired by C++ and Python, the syntax of SIN should be pretty familiar to anyone who has used these languages (or related languages like Java and Rust) in the past. SIN takes various elements from each, sometimes repurposing them; while this may, in some cases, cause confusion, the language syntax is designed to be clear and consistent.

Like other C-style languages, and being an imperative language, a SIN program is comprised of a series of *statements*, each containing any number of *expressions*.

### Statements

Statements in SIN must begin with either a [statement keyword](Language%20Keywords) or the `@` operator to call a `void`-returning function. Note only `void`-returning functions may be appear as standalone statements; value-returning functions are considered to be expressions, and so it is illegal for them to stand alone.

#### Scope Blocks

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
    else                // ERR: unexpected keyword 'else'
        let y = 0;
        let x = 0;      // not part of the else block

We will ultimately have two errors, one syntactic and one logical. The syntax error is that, since we don't use braces, the statement `let x = 30` will always be executed, as it is not part of the if-else block, meaning once the parser hits the `else` statement, there is a stray keyword in the program (and `else` is not a valid beginning to a statement). The logical error is that, barring the syntactic error, the statements `let x = 30` and `let x = 0` will *always* execute in this program because they are not a part of the branch block; the branches may only be one statement each. To get around this, curly braces may be used to create a single "statement" that consists of a chunk of code.

Also make note that in SIN, unlike C, Python, etc., the equality operator `=` and the assignment operator `=` are the same -- in other C-style languages, the assignment operator is `=` while equality is `==`, which causes confusion among new programmers. Since SIN always uses `let` and `move` for assignment, the need for the double-equals was eliminated.

### Expressions

Like other C-style languages, statements in SIN rely on various types of *expressions.* The available expression types are:

* _Call expressions:_ A function call that returns a value to be used as an expression, such as in the statement `let x = @my_func();`
* _Unary expressions:_ Expressions like `-3` or `$x`, which use a unary operator
* _Binary expressions:_ Expressions using a binary operator, like `1 + 2 * 3`
* _Symbols:_ Any symbol that must be resolved to a value somewhere in memory, such as in the statement `let x = y`, where `y` is the symbol expression
* _Literals:_ Every type has literal values; `int` has expressions like `1234`, `float` has ones like `3.14159`, and `string` has `"hello, world!"`
* _Keyword expressions:_ Keyword expressions are limited in their use and are typically used with the binary member selection operator; for example, `int:size` is a binary expression with keyword expressions on the left and right sides

### Operators and Precedence

Operator precedence in SIN closely follows operator precedence in C and Python -- and, actually, those languages were used to determine SIN operator precedence. For information on the operators in the language, see the [appropriate relevant page](Operators).

## Scopes

Like C, SIN programs are divided into a series of _scopes;_ these can be in loops, functions, or even stand alone. Scopes allow for localized data that gets destroyed when the scope is exited. For example:

    {
        alloc int x;
        {
            let x = 30; // updates to variable in higher scope is OK; these values are maintained
            alloc int y;
        }
        let y = 30; // ERR: symbol 'y' doesn't exist, as it was declared in a lower scope
    }
