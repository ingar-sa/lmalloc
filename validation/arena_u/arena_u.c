#include "arena_u.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define POWER_OF_TWO(n) (n != 0 && (n & (n - 1)) == 0)

#define PADDING(value, alignment) \
	((alignment - (value % alignment)) % alignment)

void arena_u_init(arena_u *a, bool contiguous, size_t alignment, size_t cap,
		  uint8_t *mem)
{
	a->contiguous = contiguous;
	a->cur = 0;
	a->cap = cap;
	a->mem = mem;

	if (POWER_OF_TWO(alignment))
		a->alignment = alignment;
	else
		// TODO(ingar): Error handling?
		a->alignment = 16;
}

arena_u *arena_u_create(size_t cap, bool contiguous)
{
	arena_u *arena;
	uint8_t *mem;

	if (contiguous) {
		// TODO(ingar): query cache line size
		// Ensures the arena and its data don't share a cache line
		size_t arena_cache_aligned_sz =
			sizeof(arena_u) + PADDING(sizeof(arena_u), 64);
		size_t allocation_sz = arena_cache_aligned_sz + cap;

		arena = malloc(allocation_sz);
		mem = (uint8_t *)arena + arena_cache_aligned_sz;
	} else {
		arena = malloc(sizeof(arena_u));
		mem = malloc(cap);
	}

	arena_u_init(arena, contiguous, 16, cap, mem);
	return arena;
}

void arena_u_destroy(arena_u **ap)
{
	if (ap && *ap) {
		arena_u *a = *ap;
		if (a->contiguous) {
			free(a);
		} else {
			if (a->mem)
				free(a->mem);
			//else // TODO(ingar): error logging?

			free(a);
		}

		*ap = NULL;
	}
}

/**
 * Arena allocation
 */
void *arena_u_alloc(arena_u *a, size_t size)
{
	size_t aligned_sz = size + PADDING(size, a->alignment);
	if (a->cur + aligned_sz <= a->cap) {
		void *allocation = a->mem + a->cur;
		a->cur += aligned_sz;
		return allocation;
	}
	return NULL;
}

/**
* Arena allocation with memory always cleared to zero.
*/
void *arena_u_alloc0(arena_u *a, size_t size)
{
	size_t aligned_sz = size + PADDING(size, a->alignment);
	if (a->cur + aligned_sz <= a->cap) {
		void *allocation = a->mem + a->cur;
		a->cur += aligned_sz;

		// TODO(ingar): Might need to use memset_s to guarantee clearing with optimizations
		memset(allocation, 0, aligned_sz);
		return allocation;
	}

	return NULL;
}

void arena_u_free(arena_u *a)
{
	a->cur = 0;
}

void arena_u_pop(arena_u *a, size_t size)
{
	if (((size % a->alignment) == 0) && (a->cur - size >= 0)) {
		a->cur -= size;
	}
}

void arena_u_set_alignment(arena_u *a, size_t alignment)
{
	if (POWER_OF_TWO(alignment))
		a->alignment = alignment;
	else
		// TODO(ingar): Error handling
		a->alignment = 16;
}

void *arena_u_pos(arena_u *a)
{
	void *pos = a->mem + a->cur;
	return pos;
}

void *arena_u_seek(arena_u *a, size_t pos)
{
	if (pos <= a->cap && (pos % a->alignment) == 0) {
		a->cur = pos;
		return a->mem + a->cur;
	}

	return NULL;
}
