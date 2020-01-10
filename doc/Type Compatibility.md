# SIN Documentation
## Type Compability

In SIN, there are fairly strict rules surrounding type compatibility. To understand how type compatibility operates in SIN, one must understand how SIN types are structured.

All SIN types are composed of a *primary type,* a *width,* a *sign,* *storage and access specifiers,* and possibly a *subtype* which is itself a complete type. All of these factors play into whether two types are compatible. Further, whether types are compatible depends on the order they appear (type ```A``` and type ```B``` may be compatible like ```let A = B```, but not ```let B = A```); this is typically due to the storage and access specifiers.

## Rules

Here is a general breakdown of the rules regarding type compatibility. They will be followed with examples.

### Primary Type Comparison

In order for two types to be comptible, their primary types must be compatible. This means that one of three things must be true:

* The primary types are the same *and* secondary types are compatible
* One of the primary types is ```raw< N >```

Note that this does not take into account whether you are using an operator like ```$``` or ```[]```; these operators change the expression type, and so are evaluated on the basis of the operator's return type *and* the symbol's type, not the symbol's type alone.

### Width Comparison

Types of differing widths can be compatible, but a warning will be generated indicating a loss of data is possible when converting from a larger to a smaller type (e.g., trying to store a ```long int``` in an ```int```).

### Sign Comparison

Signed and unsigned types are considered compatible, but like differing-width types, a compiler warning will be generated indicating that there is a possible data loss when the two types interact.

### Storage and Access Specifiers

In general, these will not cause too many problems with type compatibility *except* in the case of assignment. Note that a hierarchy exists among `final`, `const`, and neither, and so this must be followed (lest a compiler error pops up). See the pointer documentation for more.
