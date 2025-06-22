# lmalloc research project

lmalloc (linux memory allocation) is a research project looking into the potential benefits of implementing specialized allocators, such as arenas and pools, in the Linux kernel that are directly available in userspace.
Currently, specialized allocators are either implemented at the language level (see Odin and Zig for examples) or as libraries written in a language. The backing memory for the allocators still has to be acquired through 
the good ol' malloc & co. or mmap (there are others, but they are not frequently used.) This means that the kernel still manages the memory for the userspace allocators as if they are general heap allocations or memory mappings,
which might incur unnecessary overhead for simple allocators like arenas. Also, if the kernel knows that some memory is used in a specific way, there might be assumptions one can make that can simplify memory management of that type.
This project aims to investigate these questions.
