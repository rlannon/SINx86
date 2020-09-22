# SIN Documentation

## Divergent References

A problem that arises when using dynamically-allocated and resizeable memory is that references to said memory are not reliable. In C++, for example, when using `std::vector`, references will become invalidated if the object needs to be resized. A similar problem arises in SIN, albeit with some slight differences.

Take the following example:

    // assign a string with a very short length
    alloc string s: "abc";
    alloc ptr<string> p: $s;

    // Assign a much longer string to ensure a reallocation will occur
    let s = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. In accumsan leo est, in lobortis orci cursus sed. Integer ex odio, egestas non nunc non, consequat efficitur nibh. Donec ex neque, porttitor eu cursus vel, convallis at magna. Nunc est est, malesuada a velit et, laoreet elementum est. Nullam a porta arcu, nec mattis turpis. In molestie ac tortor vitae facilisis. Curabitur pellentesque malesuada libero, ac elementum eros. Mauris sem felis, blandit a est et, hendrerit malesuada diam. Duis accumsan metus sapien, sit amet consectetur nisl dapibus quis. Sed ornare venenatis ex vel porta. Nam porta orci dui, ut condimentum quam consequat et. Nam aliquam pulvinar arcu, non laoreet purus tristique feugiat. Vivamus vitae leo leo. Aliquam dignissim malesuada orci non pulvinar. Vivamus sapien nisl, maximus ac posuere a, hendrerit nec turpis.";

    @print(*p);

Exactly what will be printed? In some languages, the reference in `p` would be invalid and potentially unsafe. However, the reference doesn't get updated on a resize, as the MAM doesn't keep tabs on _where_ references are, just _how many._ Instead, this is a case of a _divergent reference;_ initially, the referents were the same, but a resize resulted in a reallocation, and thus caused them to refer to different objects -- they diverged.

In this case, `abc` will be printed onto the screen. While the string was resized, and the reference in `s` updated, the reference count for the old string was still nonzero, meaning it couldn't actually be freed. As such, `p` still references the initial string object to which it was assigned, and `s` contains a new object. If we invoke `free` on the pointer, the reference count will hit zero and the string object containing the text `abc` will be freed.

While divergent references make SIN a little more memory-safe (references to potentially resizing memory will not be truly invalidated), it can lead to bloat due to continually increasing dynamic memory without freeing old copies of objects. Further, although a programmer might expect a pointer to maintain a reference to a particular object, an expected update to some data might not occur. For example, adding onto the above code:

    let *p = "Hello, world!";
    print(s);

In this case, the lorem ipsum text will be printed; the update occurred to the _old_ object, not the new one, because the references diverged.

Due to the nature of divergent references, when using pointers to types that may be reallocated automatically (such as `string` or `dynamic array`), a compiler warning will be generated to alert the programmer of this possibility.
