# lmalloc research project

lmalloc (linux memory allocation) is a research project looking into the potential benefits of implementing specialized allocators, such as arenas and pools, in the Linux kernel that are directly available in userspace.
Currently, specialized allocators are either implemented at the language level (see Odin and Zig for examples) or as libraries written in a language. The backing memory for the allocators still has to be acquired through 
the good ol' malloc & co. or mmap (there are others, but they are not frequently used.) This means that the kernel still manages the memory for the userspace allocators as if they are general heap allocations or memory mappings,
which might incur unnecessary overhead for simple allocators like arenas. Also, if the kernel knows that some memory is used in a specific way, there might be assumptions one can make that can simplify memory management of that type.
This project aims to investigate these questions.

### Dependencies

lmalloc requires a copy of the project's Linux and GCC forks to be present in the top-level directory. These can be found here:

**Linux fork: [linux](https://github.com/ingar-sa/linux)**

**GCC fork: [gcc](https://github.com/ingar-sa/gcc)**

The `.gitignore` file has *linux* and *gcc* as entries so you can safely clone them into the repository.
It is ***highly*** recommended that you clone with `--depth=1` as part of your clone command. GCC has 216,000 commits and Linux has 1,300,000.
You really don't need all of them.

### Why are they not submodules?
Because submodules.

### Bachelor's thesis
The research project is for me and my group's bachelor's thesis. The repository containing stuff for the thesis itself can be found here:

**Bachelor repository: [Bachelor](https://github.com/ingar-sa/Bachelor)**
