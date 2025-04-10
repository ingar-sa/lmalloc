#include <src/lm.h>

LM_LOG_GLOBAL_DECLARE();
LM_LOG_REGISTER(u_arena);

#include <src/utils/system_info.h>
#include <src/metrics/timing.h>

#include "u_arena.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static size_t arena_cache_aligned_sz(void)
{
	size_t cacheln_sz = get_l1d_cacheln_sz();
	size_t arena_cache_aligned_sz =
		sizeof(UArena) + LmPaddingToAlign(sizeof(UArena), cacheln_sz);
	return arena_cache_aligned_sz;
}

void u_arena_init(UArena *a, bool contiguous, bool mallocd, size_t alignment,
		  size_t page_sz, size_t cap, uint8_t *mem)
{
	LmAssert(
		LmIsPowerOfTwo(alignment) && 8 <= alignment &&
			alignment <= page_sz,
		"Provided alignment is not a power of two and/or aligment not in [8, %zd]",
		page_sz);

	a->contiguous = contiguous;
	a->mallocd = mallocd;
	a->alignment = alignment;
	a->page_sz = page_sz;
	a->cur = 0;
	a->cap = cap;
	a->mem = mem;
}

UArena *u_arena_create(size_t cap, bool contiguous, bool mallocd,
		       size_t alignment)
{
	UArena *arena;
	uint8_t *mem;
	size_t page_sz = get_page_size();
	size_t cacheln_sz = get_l1d_cacheln_sz();
	size_t arena_cache_aligned_sz =
		sizeof(UArena) + LmPaddingToAlign(sizeof(UArena), cacheln_sz);

	if (mallocd) {
		if (contiguous) {
			size_t allocation_sz = arena_cache_aligned_sz + cap;
			arena = malloc(allocation_sz);
			mem = (uint8_t *)arena + arena_cache_aligned_sz;
		} else {
			arena = malloc(sizeof(UArena));
			mem = malloc(cap);
		}
	} else {
		if (cap % page_sz != 0) {
			LmLogWarning(
				"Creating an mmap'd arena of a size that is not a multiple of page size is disallowed");
			return NULL;
		}
		if (contiguous) {
			size_t allocation_sz = arena_cache_aligned_sz + cap;
			mem = mmap(NULL, allocation_sz, PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			arena = (UArena *)mem;
			mem += arena_cache_aligned_sz;
		} else {
			arena = malloc(sizeof(UArena));
			mem = mmap(NULL, cap, PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		}
	}

	u_arena_init(arena, contiguous, mallocd, alignment, page_sz, cap, mem);
	return arena;
}

void u_arena_destroy(UArena **ap)
{
	if (ap && *ap) {
		UArena *a = *ap;
		if (!a->mem) {
			LmLogWarning("Arena memory was NULL");
			return;
		}
		if (a->mallocd) {
			if (a->contiguous) {
				free(a);
			} else {
				free(a->mem);
				free(a);
			}
		} else {
			if (a->contiguous) {
				munmap(a, arena_cache_aligned_sz() + a->cap);
			} else {
				munmap(a->mem, a->cap);
				free(a);
			}
		}
		*ap = NULL;
	}
}

inline void *u_arena_alloc(UArena *a, size_t size)
{
	void *ptr = NULL;
	size_t aligned_sz = size + LmPaddingToAlign(size, a->alignment);
	if (LM_LIKELY(a->cur + aligned_sz <= a->cap)) {
		ptr = a->mem + a->cur;
		a->cur += aligned_sz;
	}

	return ptr;
}

// NOTE: (isa): See 'poc/page_zalloc/u_arena.c' for a short
// discussion on why zeroing individual allocations is better
// than pre-zeroing larger chunks
inline void *u_arena_zalloc(UArena *a, size_t size)
{
	void *ptr = NULL;
	size_t aligned_sz = size + LmPaddingToAlign(size, a->alignment);
	if (LM_LIKELY(a->cur + aligned_sz <= a->cap)) {
		ptr = a->mem + a->cur;
		a->cur += aligned_sz;
		explicit_bzero(ptr, aligned_sz);
	}
	return ptr;
}

inline void *u_arena_falloc(UArena *a, size_t size)
{
	size_t aligned_sz = size + LmPaddingToAlign(size, a->alignment);
	void *ptr = a->mem + a->cur;
	a->cur += aligned_sz;
	return ptr;
}

inline void *u_arena_fzalloc(UArena *a, size_t size)
{
	size_t aligned_sz = size + LmPaddingToAlign(size, a->alignment);
	void *ptr = a->mem + a->cur;
	a->cur += aligned_sz;
	explicit_bzero(ptr, aligned_sz);
	return ptr;
}

void u_arena_release(UArena *a, size_t pos, size_t size)
{
	LmAssert(pos < a->cur, "pos > arena pos");
	LmAssert(pos % a->page_sz == 0, "pos is not a multiple of page size");
	LmAssert(size % a->page_sz == 0, "size is not a multiple of page size");
	if (LM_UNLIKELY(a->mallocd)) {
		LmLogWarning("Cannot call u_arena_release on mallocd arena");
	} else {
		if (madvise(a->mem + pos, size, MADV_DONTNEED) != 0)
			LmLogError("madvise failed: %s", strerror(errno));
	}
}

inline void u_arena_free(UArena *a)
{
	a->cur = 0;
}

inline void u_arena_pop(UArena *a, size_t size)
{
	if (LM_LIKELY(((size % a->alignment) == 0) &&
		      ((ssize_t)a->cur - (ssize_t)size >= 0))) {
		a->cur -= size;
	}
}

void u_arena_set_alignment(UArena *a, size_t alignment)
{
	if (LmIsPowerOfTwo(alignment) && alignment <= 4096)
		a->alignment = alignment;
	else
		LmLogWarning(
			"Specified alignment is not a power of two or is > 4096");
}

inline void *u_arena_pos(UArena *a)
{
	void *pos = a->mem + a->cur;
	return pos;
}

inline void *u_arena_seek(UArena *a, size_t pos)
{
	if (LM_LIKELY(pos <= a->cap && (pos % a->alignment) == 0)) {
		a->cur = pos;
		return a->mem + a->cur;
	}

	return NULL;
}
