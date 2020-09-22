# SIN Documentation

## Struct Methods

_Note: this document describes a feature which is not yet fully-supported by the compiler._

Like many other programming languages, structs support member functions, or methods, that execute in reference to a particular structure. There is no special syntax for struct methods except for that they must be declared or defined inside a struct. If declared in a struct, they may be defined elsewhere using the scope resolution operator (`::`), like in C++. For example:

    // mystruct.sinh
    def struct myStruct {
        alloc int size;
        alloc string name;

        decl string get_name();
    }

    // mystruct.sin
    include "mystruct.sinh";

    def string myStruct::get_name() {
        return this.name;
    }

### `this` parameter

Like C++ and Java, the keyword `this` is used to denote the structure in reference to which the method is being called. By default, `this` is of the type `ref<T>`, but the type may be explicitly stated and changed to `ptr<T>`; for example:

    def string myStruct::get_name(alloc ref<myStruct> this)

or

    def string myStruct::get_name(alloc ptr<myStruct> this)

However, if `ptr` is used, then the appropriate syntax must be used; instead of `this.name`, you must say `(*this).name`.

This reflects the reality that `this` is always the first parameter to a non-static method, and will be passed in `rsi` under [SINCALL](Calling%20Convention.md). Because of this, it is actually possible in SIN to call methods without the use of the dot operator; for example:

    alloc myStruct m: construct {
        size: 0,
        name: "m",
    }

    m.get_name();
    myStruct::get_name(m);

Both of these syntaxes are acceptable. This is because `this` is the first parameter, but one which is resolved with the dot operator if the Simula-style syntax is used (`struct.member`). This is considered syntactic sugar in SIN. However, since the dot operator syntax is much more widely-known, it is generally better to utilize it.

Note that declaring methods outside of a class is different; non-member functions may not use the keyword `this`, they may not access private members, and they will undergo different name decoration (thus meaning their names have more limited capability for reuse).

### Name decoration

Names of member functions are decorated like follows:

    SIN_<scope name>_<function name>

So, using the above example, `myStruct::get_name` is decorated as `SIN_myStruct_get_name` in the generated assembly. This is to allow method names that shadow names in other scopes.
