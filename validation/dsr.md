# Overview of DSR design cycles

## Cycle 1
### Userland arena
- The first implementation allocates backing memory through malloc.
- A separate version that instead allocates backing memory through mmap should be made.
- Rationale: mmap is a more direct way of allocating memory from the kernel. 
    Allocation through malloc gives a layer of indirection and incurs the overhead from malloc's 
    internal data structures. Using mmap for the arena's backing memory provides a clearer separation
    from malloc for comparisons. The arena's malloc'd memory could cause inefficiencies for malloc
    itself, making the comparison unfair.
- Since this is not part of the treatment, it can be made without starting a new DSR cycle.
- Such an arena can also implement memory use optimizations to a larger degree by using madvise.
