# SIN Documentation

## The Memory Allocation Manager (MAM)

In SIN, memory management is typically done with the `alloc` and `free` keywords in combination with location specifiers like `dynamic`. Unlike C, these are done with *keywords* instead of library functions; the memory management scheme is integral to the langauge's design. However, this means that some library support is still required, even though it is not visible to the programmer. Rather, it is done automatically by SIN's memory management system, called the Memory Allocation Manager, or MAM. The MAM is a crucial part of SIN runtime memory management provided by the [SRE](SIN%20Runtime%20Environment).

The MAM's key job is to ensure safe allocation and release of memory as well as garbage collection through reference counting. This removes the responsibility of manual memory management, as the MAM will handle freeing dynamically-allocated memory through reference counting. Note that if the user writes wrappers for the C functions `malloc()` and `free()`, and forgoes the use of the `alloc`, `dynamic`, and `free` keywords, the MAM will not be able to help protect against the sorts of errors that are possible when manually managing memory at this level. Further, if the programmer manually invokes the `free` keyword, the compiler will issue a warning stating that there may be dangling references that would normally be cleaned up automatically. SIN *allows* programmers this freedom, but encourages them to use the tools provided by the language.

### Use in resource allocation

Whenever we wish to allocate large chunks of memory from the OS, it is generally a good idea to utilize dynamic memory rather than the stack. Dynamic memory also allows the programmer to extend the lifetime of a resource by passing references to it between scopes without that resource being automatically reclaimed (unlike stack-allocated objects). Further, dynamic memory allows for things like dynamically-sized arrays in SIN, which are not allowed when using automatic memory.

When working with dynamic memory, a lot can go wrong, and so SIN abstracts a lot of these gritty details away into the SRE, specifically the MAM. The `dynamic` keyword in SIN eventually compiles into calls to `malloc()` (or `calloc()`), and any errors in obtaining dynamic memory are handled by the MAM and passed on to the user. The MAM also tracks the allocated objects and ensures that the program never attempts to call `free()` on memory that was not obtained by `malloc()`, something which can cause program crashes in C.

### Use in the release of resources

The main purpose of the MAM is to serve as a form of garbage collection, using a reference counter on all heap resources. This allows the programmer to use `dynamic` without worrying about invoking `free` on that resource. Although this is still allowed, programmers are encouraged to just let the MAM do everything and clean up the heap once resources become inaccessible. Using `free` is disouraged for this reason (though not illegal), and using it will yield a warning stating as much.

Further, the MAM prevents the programmer from trying to free a resource that has already been acquired with `malloc()`, or that has already been freed. In SIN, unlike C, `free` may be invoked on any non-static memory (invoking `free` on a static member will generate a compiler error, and if invoked on a pointer to static memory, will have no effect), though it will only *actually* release memory back to the OS if it was allocated dynamically (stack-allocated memory, for example, will simply be unusable after a call to `free`, but will not allow reallocation of that stack space until the scope is left). A typical call to free would be something like:

    alloc array<int> x &dynamic: {0, 1, 2, 3};
    // perform some operations on the array...
    free x;

`alloc` will ultimately result in a call to the C function `malloc()`, and likewise, `free` will ultimately call the C function `free()`. In this case, memory will be released to the OS. If we call `free` again on the same variable, the compiler might warn us that it found an earlier invocation of `free`, but nothing bad will happen at runtime. This is because of the MAM's role in ensuring safe allocation and release of resources.

Further, when we free stack-allocated memory, nothing will go wrong. See the following example:

    alloc int p: 3;
    alloc int x: 2;
    let x += p;
    free p;

While such an operation would definitely give us an error in its equivalent C code, SIN is perfectly fine with it. However, the compiler may issue a warning (or a note) telling the programmer that the statement has no effect beyond disallowing access to it after the `free` statement. Further, when we do something like:

    alloc ptr<array<int>> p: $some_var;
    free *p;

First, note the difference in semantics between using `free` on a pointer in SIN and in C; in SIN, you free the data at the pointer, not the pointer itself. Second, because we don't know whether `some_var` refers to dynamic, automatic, or static memory (the compiler will not attempt to analyze where the pointed-to value actually lives in memory); so how can we be sure the `free` statement won't generate an error in the runtime library's call to `free()`?

`free` is supposed to be a safe statement in SIN, meaning it will not generate runtime errors or have undefined behavior in any use case. In order to accomplish this behavior, the MAM is used. The behavior to ensure safety in this case is as follows:

* Whenever a call to `malloc()` is performed, the MAM saves the returned address in a table
* When a `free` statement is used, the MAM looks to see if the memory address we are trying to free was one returned by `malloc()`
* If the address was returned by `malloc()`, the MAM calls `free()` on the address and deletes the entry from the table
* If the address was *not* returned by `malloc()`, the MAM ignores the `free()` statement

#### Using `free` manually

Note that using `free`, while safe from *certain* runtime errors, is discouraged. This is because it is possible for there to be dangling references after the programmer invokes it. For example:

    alloc dynamic array<5, int> a: {0, 1, 2, 3, 4};
    alloc ref<array<int>> r: $a;
    free a;
    // 'r' is now a dangling reference; it refers to memory that was freed

In order to avoid segmentation violations (or any erratic behavior that comes from reading deallocated memory), the MAM is actually allowed to simply *ignore* manual `free` calls in some cases. If the number of references to the memory we are freeing is greater than 1, the MAM can simply ignore the request to free it. While the compiler will consider the variable to be inaccessible, all references that exist will still be valid until the resource becomes truly inaccessible. However, use of `free` will not always be ignored due to the possibility of the presence of reference cycles; any resource with a reference count of 1 will be freed when invoked.

Further, note that once `free` is used in a branch, *all* subsequent accesses of that data will be considered illegal at compile-time *whether or not the resource was actually freed in that branch.* The compiler considers that data to be unsafe to use because it *might have* been freed, and so will forbid resource access (**NB:** if the access is made through a pointer or reference, the compiler won't know as it will never attempt to analyze what is being referenced). For example:

    alloc dynamic int d: 5;
    if (a * 2 >= 10) {
        free a;
    } else {
        let a = 20;
    }

    @print(a as string);    // illegal; 'a' may have been freed

When the MAM is allowed to handle freeing resources, this problem goes away.

### Reference Cycles

Consider the following:

    decl struct B;
    def struct A {
        alloc ptr<B> b;
    }

    def struct B {
        alloc ptr<A> a;
    }

    def int main() {
        alloc A a &dynamic;
        alloc B b &dynamic;
        let a.b = $b;
        let b.a = $a;
        return 0;
    }

When the `return` statement is hit, both `a` and `b` have a reference count of 2; 1 each for their named values and another each for the pointers in the other struct. In this case, a reference cycle can be easily dealt with due to the way the compiler handles freeing `struct` types -- it invokes `free` on all members first. This means:

* `a` gets freed:
  * `free` is invoked on `a.b`, decreasing `b`'s RC to 1
  * Named struct `a` was dynamically allocated, so it is freed as well, decreasing `a`'s RC to 1
* `b` gets freed:
  * `free` is invoked on `b.a`, decreasing `a`'s RC to 0, resulting in its deallocation
  * Named struct `b` was dynamically allocated, so it is freed as well, decreasing `b`'s RC to 0, resulting in its deallocation

However, one should not *rely* on the MAM, at least in its current version, to find and break such reference cycles; this is why `free` is still allowed, and sometimes necessary.

_Note: a future version of the compiler/MAM may utilize a more complex reference counting/garbage collection algorithm that can break reference cycles; for the time being, however, memory may need to be freed manually in such cycles._
