# SIN Documentation

## Operators

This document is to serve as a reference to the operators available in SIN. It describes their uses and precedence level (where 0 is the lowest precedence). Some operators use keywords, some symbols, some a combination of both (such as `let x = y`, where the operator is technically `let ... =`).

Here is a table listing the operators and their respective information:

| Operator | Name | Precedence | Associativity | Notes |
| -------- | ---- | ---------- | ------------- | ----- |
| `move ... ->` | Move assignment | 0 | Left-to-right | Moves data between locations where possible (by updating references), otherwise a copy is performed. Not yet implemented. |
| `move ... <-` | Move assignment | 0 | Right-to-left | See `->` operator |
| `let ... =` | Copy assignment | 0 | Right-to-left | Adequate for almost all assignments. Copies data between locations. Also includes all other assignment-by operators (`+=`, `-=`, `*=`, `/=`, `%=`. `&=`, `|=`, `^=`). |
| `or` | Logical inclusive OR | 1 | Left-to-right | |
| `and` | Logical AND | 1 | Left-to-right | |
| `xor` | Logical XOR | 1 | Left-to-right | |
| `|` | Bitwise-OR | 2 | Left-to-right | |
| `&` | Bitwise-AND | 2 | Left-to-right | |
| `^` | Bitwise-XOR | 2 | Left-to-right | |
| `<` | Less-than | 3 | Left-to-right | |
| `<=` | Less-or-equal | 3 | Left-to-right | |
| `>` | Greater-than | 3 | Left-to-right | |
| `>=` | Greater-or-equal | 3 | Left-to-right | |
| `=` | Equal | 3 | Left-to-right | |
| `!=` | Not equal | 3 | Left-to-right | |
| `<<` | Bitwise left shift | 4 | Left-to-right | May only be used on integral types |
| `>>` | Bitwise right shift | 4 | Left-to-right | May only be used on integral types |
| `+` | Binary plus | 5 | Left-to-right | |
| `-` | Binary minus | 5 | Left-to-right | |
| `*` | Multiplication | 6 | Left-to-right | |
| `/` | Division | 6 | Left-to-right | |
| `%` | Modulo/remainder | 6 | Left-to-right | |
| `as` | Typecast | 7 | Left-to-right | |
| `$` | Address-of | 7 | Right-to-left | |
| `*` | Dereference | 7 | Right-to-left | |
| `not` | Logical not | 7 | Right-to-left | |
| `~` | Bitwise-not | 7 | Right-to-left | |
| `+` | Unary plus | 7 | Right-to-left | |
| `-` | Unary minus | 7 | Right-to-left | |
| `:` | Attribute selection | 8 | Left-to-right | |
| `@` | Control transfer | 9 | Right-to-left | Used for function calls |
| `[]` | Array and string subscripting | 9 | Right-to-left | |
| `()` | Procedure operator | 10 | Right-to-left | Used to determine that the expression is a procedure |
| `.` | Member selection | 10 | Left-to-right | Used for tuples and structs |
| `::` | Scope resolution | 11 | None | |
