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

void ua_init(UArena *ua, bool contiguous, bool mallocd, bool bootstrapped,
	     size_t alignment, size_t page_sz, size_t cap, uint8_t *mem)
{
	LmAssert(
		LmIsPowerOfTwo(alignment) && 8 <= alignment &&
			alignment <= page_sz,
		"Provided alignment is not a power of two and/or aligment not in [8, %zd]",
		page_sz);

	ua->flags = 0;
	if (contiguous)
		UaSetIsContiguous(ua->flags);
	if (mallocd)
		UaSetIsMallocd(ua->flags);
	if (bootstrapped)
		UaSetIsBootstrapped(ua->flags);
	ua->alignment = alignment;
	ua->page_sz = page_sz;
	ua->cur = 0;
	ua->cap = cap;
	ua->mem = mem;
}

UArena *ua_create(size_t cap, bool contiguous, bool mallocd, size_t alignment)
{
	UArena *ua;
	uint8_t *mem;
	size_t page_sz = get_page_size();
	size_t cacheln_sz = get_l1d_cacheln_sz();
	size_t arena_cache_aligned_sz =
		sizeof(UArena) + LmPaddingToAlign(sizeof(UArena), cacheln_sz);

	if (mallocd) {
		if (contiguous) {
			size_t allocation_sz = arena_cache_aligned_sz + cap;
			ua = malloc(allocation_sz);
			mem = (uint8_t *)ua + arena_cache_aligned_sz;
		} else {
			ua = malloc(sizeof(UArena));
			mem = malloc(cap);
		}
	} else {
		if (cap % page_sz != 0) {
			size_t padding = LmPaddingToAlign(cap, page_sz);
			LmLogWarning(
				"Creating an mmap'd arena of a size that is "
				"not a multiple of page size is disallowed. "
				"Padding %zd bytes with %zd to align to page size",
				cap, padding);
			cap += padding;
			//return NULL;
		}
		if (contiguous) {
			size_t allocation_sz = arena_cache_aligned_sz + cap;
			mem = mmap(NULL, allocation_sz, PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
			ua = (UArena *)((uintptr_t)mem);
			mem += arena_cache_aligned_sz;
		} else {
			ua = malloc(sizeof(UArena));
			mem = mmap(NULL, cap, PROT_READ | PROT_WRITE,
				   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		}
	}

	ua_init(ua, contiguous, mallocd, false, alignment, page_sz, cap, mem);
	return ua;
}

void ua_destroy(UArena **uap)
{
	if (uap && *uap) {
		UArena *ua = *uap;
		if (UaIsBootstrapped(ua->flags))
			return;
		if (!ua->mem) {
			LmLogWarning("Arena memory was NULL");
			return;
		}
		if (UaIsMallocd(ua->flags)) {
			if (UaIsContiguous(ua->flags)) {
				free(ua);
			} else {
				free(ua->mem);
				free(ua);
			}
		} else {
			if (UaIsContiguous(ua->flags)) {
				munmap(ua, arena_cache_aligned_sz() + ua->cap);
			} else {
				munmap(ua->mem, ua->cap);
				free(ua);
			}
		}
		*uap = NULL;
	}
}

UArena *ua_bootstrap(UArena *ua, UArena *new_existing, size_t cap,
		     size_t alignment)
{
	UArena *new;
	if (new_existing)
		new = new_existing;
	else
		new = UaPushStruct(ua, UArena);

	uint8_t *mem = ua_zalloc(ua, cap);
	if (!new || !mem) {
		LmLogWarning("Insufficient memory to bootstrap arena");
		return NULL;
	}

	ua_init(new, false, false, true, alignment, ua->page_sz, cap, mem);
	return new;
}

inline void *ua_alloc(UArena *ua, size_t size)
{
	void *ptr = NULL;
	size_t aligned_sz = size + LmPaddingToAlign(size, ua->alignment);
	if (LM_LIKELY(ua->cur + aligned_sz <= ua->cap)) {
		ptr = ua->mem + ua->cur;
		ua->cur += aligned_sz;
	}

	return ptr;
}

// NOTE: (isa): See 'poc/page_zalloc/u_arena.c' for a short
// discussion on why zeroing individual allocations is better
// than pre-zeroing larger chunks
inline void *ua_zalloc(UArena *ua, size_t size)
{
	void *ptr = NULL;
	size_t aligned_sz = size + LmPaddingToAlign(size, ua->alignment);
	if (LM_LIKELY(ua->cur + aligned_sz <= ua->cap)) {
		ptr = ua->mem + ua->cur;
		ua->cur += aligned_sz;
		explicit_bzero(ptr, aligned_sz);
	}
	return ptr;
}

inline void *ua_falloc(UArena *ua, size_t size)
{
	size_t aligned_sz = size + LmPaddingToAlign(size, ua->alignment);
	void *ptr = ua->mem + ua->cur;
	ua->cur += aligned_sz;
	return ptr;
}

inline void *ua_fzalloc(UArena *ua, size_t size)
{
	size_t aligned_sz = size + LmPaddingToAlign(size, ua->alignment);
	void *ptr = ua->mem + ua->cur;
	ua->cur += aligned_sz;
	explicit_bzero(ptr, aligned_sz);
	return ptr;
}

void ua_release(UArena *ua, size_t pos, size_t size)
{
	LmAssert(pos < ua->cur, "pos > arena pos");
	LmAssert(pos % ua->page_sz == 0, "pos is not a multiple of page size");
	LmAssert(size % ua->page_sz == 0,
		 "size is not a multiple of page size");
	if (LM_UNLIKELY(UaIsMallocd(ua->flags))) {
		LmLogWarning("Cannot call u_arena_release on mallocd arena");
	} else {
		if (madvise(ua->mem + pos, size, MADV_DONTNEED) != 0)
			LmLogError("madvise failed: %s", strerror(errno));
	}
}

inline void ua_free(UArena *ua)
{
	ua->cur = 0;
}

inline void ua_pop(UArena *ua, size_t size)
{
	if (LM_LIKELY(((size % ua->alignment) == 0) &&
		      ((ssize_t)ua->cur - (ssize_t)size >= 0))) {
		ua->cur -= size;
	}
}

inline size_t ua_pos(UArena *ua)
{
	size_t pos = ua->cur;
	return pos;
}

inline void *ua_seek(UArena *ua, size_t pos)
{
	if (LM_LIKELY(pos <= ua->cap && (pos % ua->alignment) == 0)) {
		ua->cur = pos;
		return ua->mem + ua->cur;
	}

	return NULL;
}

inline size_t ua_reserve(UArena *ua, size_t sz)
{
	if (LM_LIKELY(ua->cur + sz <= ua->cap)) {
		ua->cur += sz;
		return sz;
	} else {
		return ua->cap - ua->cur;
	}
}

void ua__thread_arenas_init__(UArena *ta_buf[], struct ua__thread_arenas__ *tas,
			      struct ua__thread_arenas__ **ta_instance)
{
	tas->uas = ta_buf;
	*ta_instance = tas;
}

void ua__thread_arenas_init_extern__(struct ua__thread_arenas__ *tas,
				     struct ua__thread_arenas__ **ta_instance)
{
	*ta_instance = tas;
}

int ua__thread_arenas_add__(UArena *a, struct ua__thread_arenas__ *ta_instance)
{
	if (ta_instance->count + 1 <= ta_instance->max_count) {
		ta_instance->uas[ta_instance->count++] = a;
		return 0;
	}

	return 1;
}

UArenaScratch ua_scratch_begin(UArena *ua)
{
	UArenaScratch uas;
	uas.ua = ua;
	uas.f5 = ua->cur;
	return uas;
}

UArenaScratch ua__scratch_get__(UArena **conflicts, int conflict_count,
				struct ua__thread_arenas__ *tas)
{
	UArena *ua = NULL;
	if (conflict_count > 0) {
		for (int i = 0; i < tas->count; ++i) {
			for (int j = 0; j < conflict_count; ++j) {
				if (tas->uas[i] != conflicts[j]) {
					ua = tas->uas[i];
					goto exit;
				}
			}
		}
	} else {
		ua = tas->uas[0];
	}
exit:
	return (ua == NULL) ? (UArenaScratch){ 0 } : ua_scratch_begin(ua);
}

void ua__scratch_release__(UArenaScratch uas)
{
	ua_seek(uas.ua, uas.f5);
}
