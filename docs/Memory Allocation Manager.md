# SIN Documentation

## The Memory Allocation Manager (MAM)

In SIN, memory management is typically done with the `alloc` and `free` keywords in combination with location specifiers like `dynamic`. Unlike C, these are done with *keywords* instead of library functions. However, this means that some library support is still required, even though it is not visible to the programmer. Rather, it is done automatically by SIN's memory management system, also called the Memory Allocation Manager, or MAM. The MAM is a crucial part of SIN runtime memory management provided by the [SRE](SIN%20Runtime%20Environment.md).

The MAM's key job is to ensure safe allocation and release of memory. Note, however, that it is *not* a full garbage collector; SIN is not a garbage-collected language, though it does use a tool (the MAM) to ensure that the user handles dynamic memory allocation safely. Note that if the user writes wrappers for the C functions `malloc()` and `free`, and forgoes the use of the `alloc`, `dynamic`, and `free` keywords, the MAM will not be able to help protect against the sorts of errors that are possible when manually managing memory at this level.

### Use in resource allocation

Whenever we wish to allocate large chunks of memory from the OS, it is generally a good idea to utilize dynamic memory rather than the stack. Further, dynamic memory allows for things like dynamically-sized arrays in SIN, something which is not allowed when using automatic memory.

When working with dynamic memory, a lot can go wrong, and so SIN abstracts a lot of these gritty details away into the SRE, specifically the MAM. The `dynamic` keyword in SIN eventually compiles into calls to `malloc()` (or `calloc()`), and any errors in obtaining dynamic memory are handled by the MAM and passed on to the user. The MAM also tracks the allocated objects and ensures that the program never attempts to call `free()` on memory that was not obtained by `malloc()`, something which can cause program crashes in C. Further,

### Use in the release of resources

A key example of this in action is the `free` keyword; in SIN, unlike C, `free` may be invoked on any non-static memory (invoking `free` on a static member will generate a compiler error, and if invoked on a pointer to static memory, will have no effect), though it will only *actually* release memory back to the OS if it was allocated dynamically (stack-allocated memory, for example, will simply be unusable after a call to `free`, but will not allow reallocation of that stack space until the scope is left). A typical call to free would be something like:

    alloc array<int> x &dynamic: {0, 1, 2, 3};
    // perform some operations on the array...
    free x;

`alloc` will ultimately result in a call to the C function `malloc()`, and likewise, `free` will ultimately call the C function `free()`. In this case, memory will be released to the OS. If we call `free` again on the same variable, the compiler might warn us that the memory was already freed, but nothing will happen at runtime. This is because of the MAM's role in ensuring safe allocation and release of resources. Further, when we free stack-allocated memory:

    alloc int p: 3;
    alloc int x: 2;
    let x += p;
    free p;

While such an operation would definitely give us an error in its equivalent C code, SIN is perfectly fine with it. However, the compiler may issue a warning (or a note) telling the programmer that the statement has no effect beyond disallowing access to it after the `free` statement. Further, when we do something like:

    alloc ptr<array<int>> p: $some_var;
    free *p;

First, note the difference in semantics between using `free` on a pointer in SIN and in C; in SIN, you free the data at the pointer, not the pointer itself.

Second, because we don't know whether `some_var` refers to dynamic, automatic, or static memory (the compiler will not attempt to analyze where the pointer value actually lives in memory); so how can we be sure the `free` statement won't generate an error in the runtime library's call to `free()`?

`free` is supposed to be a safe statement in SIN, meaning it will not generate runtime errors or have undefined behavior in any use case. In order to accomplish this behavior, the MAM is used. The behavior to ensure safety in this case is as follows:

* Whenever a call to `malloc()` is performed, the MAM saves the returned address in a hash table
* When a `free` statement is used, the MAM looks to see if the memory address we are trying to free was one returned by `malloc()`
* If the address was returned by `malloc()`, the MAM calls `free()` on the address and deletes the entry from the table
* If the address was *not* returned by `malloc()`, the MAM ignores the `free()` statement
