# SIN Documentation

## Reference Types

SIN allows for data that is referred to by the term "reference types" because although they don't use `ref< T >` in their allocation, under the hood, these types are implemented through the use of references. This includes the `string` type as well as all data marked with the `dynamic` qualifier. Such data will use pointers to data in the heap even though they are not declared as references.

**However,** the key distinction between these reference types and an actual reference is that, when passed to a function, a copy of the data will be made and passed by value. For example:

    def void v (alloc string s) {
        let s = "changed!";
        return void;
    }

    alloc string my_string: "hello, world!";
    @v(my_string);
    @print(my_string);  // prints "hello, world!"

Although `v` changes its string, `my_string` is unaffected because it was passed by value; the changes made in the function were made to the *copy* of `my_string`, because a `string` is not considered to be a reference even though it technically is. If we wanted to actually pass the string by reference, we would have to say:

    def void v (alloc ref<string> s) {
        let s = "changed!";
        return void;
    }

    alloc string my_string: "hello, world!";
    @v($my_string);
    @print(my_string);  // prints "changed!"

Note the syntactic changes. Because the parameter `s` must be a reference, we must explicitly pass `my_string` by reference, meaning a copy of the string is not made before being passed to the function. As a result, the string itself is modified.

### The `string` type

While strings are similar to `array<char>`, they are *not* identical in their behavior; `string` is a fully-fledged type while `array<char>` is a simple aggregate with markedly different behavior. The key difference is that automatic string types are still variable-length, while the same cannot be said of arrays. Arrays, unless marked as `dynamic`, are always of a fixed width which is known at compile time. Strings, on the other hand, are not, and always use dynamic (or `const`) memory. Although they may look identical in memory (4-byte length followed by characters), their behavior is different.

Strings may be marked as `dynamic`, and although this does not change where the string is located in program memory, it *does* affect the lifetime of the object; if a string is not marked as `dynamic`, a reference to it may not be returned from a function because the compiler treats it as a local variable (and returning pointers to local data is not allowed in SIN as it does not live past a function's return).

*A note about why* `string` *is a unique type:* The decision to add `string` to the language was done because of its ubiquity; since SIN's goal is to make the life of a programmer easier by reducing the use of confusing syntax and manual memory management, it makes more sense to have the `string` type rather than requiring programmers to create a `dynamic array<char>` and deal with the nightmares one might face in C (rather than, say, C++ or Python) every time they wish to utilize strings. While this may make the implementation of strings in the compiler a little more thorny, it reduces unnecessary difficulty when programming and so was deemed to belong in the language. Plus, it allows use of the concatenation operator (`+`) where `dynamic array<char>` would not.

**NB:** It is generally a very *bad* idea to use pointers and references to strings; not only is it *very rarely* necessary, but because its size tends to be so variable, any references to it may be invalidated with any reassignment.
