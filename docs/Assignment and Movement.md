# SIN Documentation

## Assignment and Movement

_Note: this document describes certain features which have not yet been implemented into the SIN compiler._

SIN supports two methods of data assignment, `let` and `move`. While they can be interchangeable, they perform slightly different functions when used with references and dynamic data. As such, it is generally advised that, to maintain clarity, programmers stick with `let` for simple assignment and data copying, and only use `move` when they intend on changing a referent and transfering object ownership.

### Assignment

Assignment in SIN is performed with the `let` keyword. For example,

    let x = y;

copies the value in `y` to the location `x`. This is important -- `let` *always copies* data. As such, something like:

    alloc array<3, int> my_arr: { 0, 1, 2 };
    alloc array<int> s2 &dynamic;
    let s2 = s;

`s2` will be reallocated to the size of `s`, and `s` will then be *copied* into `s2`. It results in two distinct, but identical, objects in dynamic memory.

Similarly, this works on strings as well. Although the `string` type uses references internally, they are not treated as references unless explicitly marked as `ref<string>` or `dynamic`. So, a string will be copied when use with `let`, such as:

    alloc string s: "hello, world!";
    alloc string s2: "";
    let s2 = s;     // copies s into s2

#### Operators with `let`

There are a few assignment operators you may use with `let`:

| Operator | Description | Expanded form |
| -------- | ----------- | ------------- |
| `=` | Simple assignment | |
| `+=` | Addition of right-hand side to left-hand side | `let a += b` expands to `let a = a + b` |
| `-=` | Subtraction of RHS from LHS | `let a -= b` expands to `let a = a - b` |
| `*=` | Multiplication of LHS by RHS | `let a = a * b` |
| `/=` | Division of LHS by RHS | `let a = a / b` |
| `%=` | Modulo of LHS by RHS | `let a = a % b` |
| `&=` | Bit-AND of LHS and RHS | `let a = a & b` |
| `|=` | Bit-OR of LHS and RHS | `let a = a | b` |
| `^=` | Bit-XOR of LHS by RHS | `let a = a ^ b` |

These compound operators are only supported with assignment; you cannot utilize them as equality operators (e.g., something like `if (a += b)` is illegal).

### Movement

_Note: not yet implemented in SIN._

Sometimes, we want to *move* data instead of *copy* it, especially if we have large dynamically-allocated objects. For non-references and non-dynamic data, this is equivalent to assignment wit `let`, but if we are utilizing references or dynamic memory, we can move the data by copying its reference into a new location. For example:

    construct some_large_struct s (10, 20, 30) &dynamic;    // dynamically allocate and initialize a struct
    alloc some_large_struct a_copy &dynamic;
    move s -> a_copy;

We can also simply do this by using the `ref< T >` type:

    alloc ref<some_large_struct> r: $s;

The benefit of using `move` over a reference, however, is that the former allows us to change the referent, whereas the referent of a `ref< T >` object may not be modified.

Further, it must be noted that `move` may only be used with modifiable-lvalues; it does not take literal arguments or other expressions. For example, the following is not allowed:

    move 30 -> a;   // illegal; cannot move literal
    move a * 2 -> b;    // illegal; cannot move arithmetic binary expression
    move a -> b;    // legal; arguments are both modifiable-lvalues
    move a -> c.x;  // legal; member selection returns a modifiable-lvalue
    move x -> *p;   // valid; moves symbol to modifiable-lvalue

#### Operators with `move`

Because the operands are so limited, `move` only accepts the following two operators:

* `->` - Move left-hand side into right-hand side; e.g., `move b -> a`
* `<-` - Move right-hand side into left-hand side; e.g,. `move a <- b`

### When to use `let` vs `move`

Sometimes, there is no difference between using `let` and using `move` on data. For example,

    alloc int a: 10;
    alloc int b;
    move a -> b;

Since `a` and `b` are not references, `move` is equivalent to `let` and performs a copy.

Where the difference between them comes in is when using things like dynamic arrays, as `let` allows data to be copied between arrays while `move` allows the reference to be moved and array to be, in effect, reallocated. For example:

    alloc array<10, int> a &dynamic = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
    alloc array<50, int> b &dynamic;
    let b = a;  // copy the elements into the new array
    move b -> a;    // move the array, updating the reference

In the above code sample, we first allocate an array of 10 elements and initialize it. This array is `dynamic`, meaning it is dynamically-sized. We then allocate a larger dynamic array, `b`, and copy all of the elements from `a` into it. Once this reference is copied, `a` and `b` refer to the same resource. This is not possible by using references alone, as a reference may not be updated.
