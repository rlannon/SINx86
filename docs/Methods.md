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

Like C++ and Java, the keyword `this` is used to denote the structure in reference to which the method is being called. Unlike C++, `this` is always a reference, not a pointer.

### Name decoration

Names of member functions are decorated like follows:

    SIN_<scope name>_<function name>

So, using the above example, `myStruct::get_name` is decorated as `SIN_myStruct_get_name` in the generated assembly. This is to allow method names that shadow names in other scopes.
