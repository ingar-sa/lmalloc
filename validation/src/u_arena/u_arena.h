/**
 * @file u_arena.h
 * @brief Arena allocator implementation
 */

#ifndef U_ARENA_H
#define U_ARENA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @struct u_arena
 * @brief Memory arena structure for efficient memory allocation
 * 
 * This structure manages a block of memory from which allocations can be made.
 * The arena allows for fast allocation and bulk deallocation of memory.
 * 
 * @var u_arena::contiguous
 * Whether the arena's metadata and memory block are allocated contiguously
 * 
 * @var u_arena::alignment
 * Byte alignment for allocations (must be a power of two)
 * 
 * @var u_arena::cap
 * Total capacity of the memory arena in bytes
 * 
 * @var u_arena::cur
 * Current position in the arena (bytes allocated so far)
 * 
 * @var u_arena::mem
 * Pointer to the memory block managed by this arena
 */
typedef struct u_arena {
	bool contiguous;
	size_t alignment;
	size_t cap;
	size_t cur;
	uint8_t *mem;
} u_arena;

/**
 * @brief Initialize an existing arena structure
 * 
 * @param a Pointer to arena structure to initialize
 * @param contiguous Whether the arena's metadata and memory are contiguous
 * @param alignment Byte alignment for allocations (must be a power of two)
 * @param cap Total capacity of the memory arena in bytes
 * @param mem Pointer to pre-allocated memory block
 */
void u_arena_init(u_arena *a, bool contiguous, size_t alignment, size_t cap,
		  uint8_t *mem);

/**
 * @brief Create a new memory arena
 * 
 * @param cap Total capacity of the memory arena in bytes
 * @param contiguous If true, allocate arena and its memory in one contiguous block
 * @return Pointer to the newly created arena, or NULL on failure
 */
u_arena *u_arena_create(size_t cap, bool contiguous, size_t alignment);

/**
 * @brief Destroy a memory arena and free all associated memory
 * 
 * @param ap Pointer to an arena pointer (will be set to NULL after freeing)
 */
void u_arena_destroy(u_arena **ap);

/**
 * @brief Allocate memory from the arena
 * 
 * @param a Pointer to the arena
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated memory, or NULL if insufficient space
 */
void *u_arena_alloc(u_arena *a, size_t size);

/**
 * @brief Allocate zero-initialized memory from the arena
 * 
 * @param a Pointer to the arena
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated memory (zeroed), or NULL if insufficient space
 */
void *u_arena_alloc0(u_arena *a, size_t size);

/**
 * @brief Allocate memory from the arena without bounds checking
 * 
 * @param a Pointer to the arena
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated memory (zeroed), or NULL if insufficient space
 */
void *u_arena_alloc_no_bounds_check(u_arena *a, size_t size);

/**
 * @brief Allocate memory from the arena without aligning
 * 
 * @param a Pointer to the arena
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated memory (zeroed), or NULL if insufficient space
 */
void *u_arena_alloc_no_align(u_arena *a, size_t size);

/**
 * @brief Allocate memory from the arena, but does no bounds checking or alignment 
 * (in the name of speed. Gotta go fast)
 * 
 * @param a Pointer to the arena
 * @param size Number of bytes to allocate
 * @return Pointer to the allocated memory (zeroed), or NULL if insufficient space
 */
void *u_arena_alloc_fast(u_arena *a, size_t size);

/**
 * @brief Reset the arena (free all allocations)
 * 
 * This operation doesn't actually free memory but resets the arena to its initial state,
 * allowing the memory to be reused for new allocations.
 * 
 * @param a Pointer to the arena
 */
void u_arena_free(u_arena *a);

/**
 * @brief Pop (deallocate) a number of bytes from the arena
 * 
 * @param a Pointer to the arena
 * @param size Size of the memory block to pop (must match the arena's alignment)
 */
void u_arena_pop(u_arena *a, size_t size);

/**
 * @brief Set the alignment for future allocations
 * 
 * @param a Pointer to the arena
 * @param alignment New alignment value (must be a power of two)
 */
void u_arena_set_alignment(u_arena *a, size_t alignment);

/**
 * @brief Get the current position in the arena
 * 
 * @param a Pointer to the arena
 * @return Pointer to the current position in the arena's memory block
 */
void *u_arena_pos(u_arena *a);

/**
 * @brief Seek to a specific position in the arena
 * 
 * @param a Pointer to the arena
 * @param pos Position to seek to (must be aligned and within capacity)
 * @return Pointer to the new position, or NULL if the position is invalid
 */
void *u_arena_seek(u_arena *a, size_t pos);

#define u_arena_array(a, type, count) u_arena_alloc(a, sizeof(type) * count)

#define u_arena_struct(a, type) u_arena_array(a, type, 1)

#endif /* u_arena_H */
