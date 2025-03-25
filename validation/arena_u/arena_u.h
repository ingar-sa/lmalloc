/**
 * @file arena_u.h
 * @brief Arena allocator implementation
 */

#ifndef ARENA_U_H
#define ARENA_U_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @struct arena_u
 * @brief Memory arena structure for efficient memory allocation
 * 
 * This structure manages a block of memory from which allocations can be made.
 * The arena allows for fast allocation and bulk deallocation of memory.
 * 
 * @var arena_u::contiguous
 * Whether the arena's metadata and memory block are allocated contiguously
 * 
 * @var arena_u::alignment
 * Byte alignment for allocations (must be a power of two)
 * 
 * @var arena_u::cap
 * Total capacity of the memory arena in bytes
 * 
 * @var arena_u::cur
 * Current position in the arena (bytes allocated so far)
 * 
 * @var arena_u::mem
 * Pointer to the memory block managed by this arena
 */
typedef struct arena_u {
	bool contiguous;
	size_t alignment;
	size_t cap;
	size_t cur;
	uint8_t *mem;
} arena_u;

/**
 * @brief Initialize an existing arena structure
 * 
 * @param a Pointer to arena structure to initialize
 * @param contiguous Whether the arena's metadata and memory are contiguous
 * @param alignment Byte alignment for allocations (must be a power of two)
 * @param cap Total capacity of the memory arena in bytes
 * @param mem Pointer to pre-allocated memory block
 */
void arena_u_init(arena_u *a, bool contiguous, size_t alignment, size_t cap,
		  uint8_t *mem);

/**
 * @brief Create a new memory arena
 * 
 * @param cap Total capacity of the memory arena in bytes
 * @param contiguous If true, allocate arena and its memory in one contiguous block
 * @return Pointer to the newly created arena, or NULL on failure
 */
arena_u *arena_u_create(size_t cap, bool contiguous, size_t alignment);

/**
 * @brief Destroy a memory arena and free all associated memory
 * 
 * @param ap Pointer to an arena pointer (will be set to NULL after freeing)
 */
void arena_u_destroy(arena_u **ap);

/**
 * @brief Allocate memory from the arena
 * 
 * @param a Pointer to the arena
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated memory, or NULL if insufficient space
 */
void *arena_u_alloc(arena_u *a, size_t size);

/**
 * @brief Allocate zero-initialized memory from the arena
 * 
 * @param a Pointer to the arena
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated memory (zeroed), or NULL if insufficient space
 */
void *arena_u_alloc0(arena_u *a, size_t size);

/**
 * @brief Reset the arena (free all allocations)
 * 
 * This operation doesn't actually free memory but resets the arena to its initial state,
 * allowing the memory to be reused for new allocations.
 * 
 * @param a Pointer to the arena
 */
void arena_u_free(arena_u *a);

/**
 * @brief Pop (deallocate) a number of bytes from the arena
 * 
 * @param a Pointer to the arena
 * @param size Size of the memory block to pop (must match the arena's alignment)
 */
void arena_u_pop(arena_u *a, size_t size);

/**
 * @brief Set the alignment for future allocations
 * 
 * @param a Pointer to the arena
 * @param alignment New alignment value (must be a power of two)
 */
void arena_u_set_alignment(arena_u *a, size_t alignment);

/**
 * @brief Get the current position in the arena
 * 
 * @param a Pointer to the arena
 * @return Pointer to the current position in the arena's memory block
 */
void *arena_u_pos(arena_u *a);

/**
 * @brief Seek to a specific position in the arena
 * 
 * @param a Pointer to the arena
 * @param pos Position to seek to (must be aligned and within capacity)
 * @return Pointer to the new position, or NULL if the position is invalid
 */
void *arena_u_seek(arena_u *a, size_t pos);

#define arena_u_array(a, type, count) arena_u_alloc(a, sizeof(type) * count)

#define arena_u_struct(a, type) arena_u_array(a, type, 1)

#endif /* ARENA_U_H */
