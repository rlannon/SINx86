# SIN Documentation

## Errors, Warnings, and Notes

The SIN compiler, like compilers for other languages, has three different types of messages that it may give the user. These are _errors, warnings,_ and _notes._ Each serves a different purpose, but will generally contain a message, a code, and a line number associated with the warning.

### Errors

Errors are issued when the programmer does something illegal. For example, if a type validity rule is violated, the compiler will issue a warning:

    alloc dynamic int my_int &static;   // Error; memory cannot be 'static' and 'dynamic' at the same time

The compiler will not make any effort to fix the error automatically, and will instead abort compilation. Errors could occur at the lex stage (e.g., a token is found that the lexer doesn't recognize), the parse stage (the parser could not properly parse the lexemes), or the code generation stage (e.g., type validity issues, undefined references, or duplicate symbols).

### Warnings

Warnings are issued when there could potentially be a problem or unintended consequence, but what is being done isn't strictly illegal. For example:

    alloc int i: 100;
    alloc short int j;
    let j = i;  // Warning: width mismatch

These are most common for things like width or sign mismatches, but could occur for potentially unsafe operations as well.

### Notes

Notes are issued when there won't be a problem with the operation, but it is unnecessary or the result of a potential oversight. This is usually to indicate that the compiler is going to ignore something or add something in automatically. For example, a note will be issued when compiling a file without the function `main`, when casting from one type to the same type, or when using qualifiers with types that don't need them (e.g., pointers).
