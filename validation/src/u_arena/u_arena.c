#include <src/lm.h>

LM_LOG_GLOBAL_DECLARE();
LM_LOG_REGISTER(arena);

#include <src/metrics/timing.h>

#include "u_arena.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void u_arena_init(u_arena *a, bool contiguous, size_t alignment, size_t cap,
		  uint8_t *mem)
{
	LmAssert(LmIsPowerOfTwo(alignment) && alignment <= 4096,
		 "Provided alignment is not a power of two");

	a->contiguous = contiguous;
	a->alignment = alignment;
	a->cur = 0;
	a->cap = cap;
	a->mem = mem;

	LmLogDebug("arena: %p, mem %p", (void *)a, (void *)mem);
}

u_arena *u_arena_create(size_t cap, bool contiguous, size_t alignment)
{
	u_arena *arena;
	uint8_t *mem;

	if (contiguous) {
		// Ensures the arena and its data don't share a cache line.
		size_t l1_cache_line_size = lm_get_l1_cache_line_size();
		LmLogDebug("L1 cache line size: %zd", l1_cache_line_size);
		size_t arena_cache_aligned_sz =
			sizeof(u_arena) +
			LmPaddingToAlign(sizeof(u_arena), l1_cache_line_size);
		size_t allocation_sz = arena_cache_aligned_sz + cap;

		arena = malloc(allocation_sz);
		mem = (uint8_t *)arena + arena_cache_aligned_sz;
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
				LmLogWarning("Arena memory pointer was NULL");

			free(a);
		}

		*ap = NULL;
	}
}

void *u_arena_alloc(u_arena *a, size_t size)
{
	void *ptr = NULL;
	size_t aligned_sz = size + LmPaddingToAlign(size, a->alignment);
	if (a->cur + aligned_sz <= a->cap) {
		ptr = a->mem + a->cur;
		a->cur += aligned_sz;
	}

	return ptr;
}

void *u_arena_alloc0(u_arena *a, size_t size)
{
	void *ptr = NULL;
	size_t aligned_sz = size + LmPaddingToAlign(size, a->alignment);
	if (a->cur + aligned_sz <= a->cap) {
		ptr = a->mem + a->cur;
		a->cur += aligned_sz;
		lm_memset_s(ptr, 0, aligned_sz);
	}

	return ptr;
}

// TODO: (isa): Might be superfluous
void *u_arena_alloc_no_bounds_check(u_arena *a, size_t size)
{
	size_t aligned_sz = size + LmPaddingToAlign(size, a->alignment);
	void *ptr = a->mem + a->cur;
	a->cur += aligned_sz;
	return ptr;
}

void *u_arena_alloc_no_align(u_arena *a, size_t size)
{
	void *ptr = NULL;
	if (a->cur + size <= a->cap) {
		ptr = a->mem + a->cur;
		a->cur += size;
	}

	return ptr;
}

void *u_arena_alloc_fast(u_arena *a, size_t size)
{
	void *ptr = a->mem + a->cur;
	a->cur += size;
	return ptr;
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
	if (LmIsPowerOfTwo(alignment) && alignment <= 4096)
		a->alignment = alignment;
	else
		LmLogWarning(
			"Specified alignment is not a power of two or is > 4096");
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
