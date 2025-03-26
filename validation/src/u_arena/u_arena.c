#include <src/eit.h>

EIT_LOG_REGISTER(arena);

#include "u_arena.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void u_arena_init(u_arena *a, bool contiguous, size_t alignment, size_t cap,
		  uint8_t *mem)
{
	EitAssert(EitIsPowerOfTwo(alignment),
		  "Provided alignment is not a power of two");

	a->contiguous = contiguous;
	a->alignment = alignment;
	a->cur = 0;
	a->cap = cap;
	a->mem = mem;

	EitLogDebug("arena: %p, mem %p", (void *)a, (void *)mem);
}

u_arena *u_arena_create(size_t cap, bool contiguous, size_t alignment)
{
	u_arena *arena;
	uint8_t *mem;

	if (contiguous) {
		// TODO(ingar): query cache line size
		// Ensures the arena and its data don't share a cache line.
		size_t arena_cache_aligned_sz =
			sizeof(u_arena) +
			EitPaddingToAlign(sizeof(u_arena), 64);
		size_t allocation_sz = arena_cache_aligned_sz + cap;

		arena = malloc(allocation_sz);
		mem = (uint8_t *)arena + arena_cache_aligned_sz;
		EitLogDebug("arena: %p, mem %p", (void *)arena, (void *)mem);
	} else {
		arena = malloc(sizeof(u_arena));
		mem = malloc(cap);
	}

	u_arena_init(arena, contiguous, alignment, cap, mem);
	return arena;
}

void u_arena_destroy(u_arena **ap)
{
	if (ap && *ap) {
		u_arena *a = *ap;
		if (a->contiguous) {
			free(a);
		} else {
			if (a->mem)
				free(a->mem);
			else
				EitLogWarning("Arena pointer was NULL");

			free(a);
		}

		*ap = NULL;
	}
}

/**
 * Arena allocation
 */
void *u_arena_alloc(u_arena *a, size_t size)
{
	size_t aligned_sz = size + EitPaddingToAlign(size, a->alignment);
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
void *u_arena_alloc0(u_arena *a, size_t size)
{
	size_t aligned_sz = size + EitPaddingToAlign(size, a->alignment);
	if (a->cur + aligned_sz <= a->cap) {
		void *allocation = a->mem + a->cur;
		a->cur += aligned_sz;

		// TODO(ingar): Might need to use memset_s to guarantee clearing with optimizations
		memset(allocation, 0, aligned_sz);
		return allocation;
	}

	return NULL;
}

void u_arena_free(u_arena *a)
{
	a->cur = 0;
}

void u_arena_pop(u_arena *a, size_t size)
{
	if (((size % a->alignment) == 0) &&
	    ((ssize_t)a->cur - (ssize_t)size >= 0)) {
		a->cur -= size;
	}
}

void u_arena_set_alignment(u_arena *a, size_t alignment)
{
	if (EitIsPowerOfTwo(alignment))
		a->alignment = alignment;
	else
		EitLogWarning("Specified alignment is not a power of two");
}

void *u_arena_pos(u_arena *a)
{
	void *pos = a->mem + a->cur;
	return pos;
}

void *u_arena_seek(u_arena *a, size_t pos)
{
	if (pos <= a->cap && (pos % a->alignment) == 0) {
		a->cur = pos;
		return a->mem + a->cur;
	}

	return NULL;
}
