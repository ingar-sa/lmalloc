#include <src/lm.h>
LM_LOG_REGISTER(allocator_wrappers);

#include <src/metrics/timing.h>

#include "u_arena.h"
#include "allocator_wrappers.h"

static uint64_t ua_alloc_total;
void *ua_alloc_wrapper_timed(UArena *ua, size_t sz)
{
	uint64_t start = start_tsc_timing();
	//--------------------------------------
	void *ptr = ua_alloc(ua, sz);
	//--------------------------------------
	uint64_t end = end_tsc_timing();
	ua_alloc_total += end - start;
	//--------------------------------------
	return ptr;
}

static uint64_t ua_zalloc_total;
void *ua_zalloc_wrapper_timed(UArena *ua, size_t sz)
{
	uint64_t start = start_tsc_timing();
	//--------------------------------------
	void *ptr = ua_zalloc(ua, sz);
	//--------------------------------------
	uint64_t end = end_tsc_timing();
	ua_zalloc_total += end - start;
	//--------------------------------------
	return ptr;
}

static uint64_t ua_falloc_total;
void *ua_falloc_wrapper_timed(UArena *ua, size_t sz)
{
	uint64_t start = start_tsc_timing();
	//--------------------------------------
	void *ptr = ua_falloc(ua, sz);
	//--------------------------------------
	uint64_t end = end_tsc_timing();
	ua_falloc_total += end - start;
	//--------------------------------------
	return ptr;
}

static uint64_t ua_fzalloc_total;
void *ua_fzalloc_wrapper_timed(UArena *ua, size_t sz)
{
	uint64_t start = start_tsc_timing();
	//--------------------------------------
	void *ptr = ua_fzalloc(ua, sz);
	//--------------------------------------
	uint64_t end = end_tsc_timing();
	ua_fzalloc_total += end - start;
	//--------------------------------------
	return ptr;
}

static uint64_t ua_realloc_total;
void *ua_realloc_wrapper_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ptr;
	uint64_t start = start_tsc_timing();
	//--------------------------------------
	void *new = ua_alloc(ua, sz);
	memcpy(new, ptr, old_sz);
	//--------------------------------------
	uint64_t end = end_tsc_timing();
	ua_realloc_total += end - start;
	//--------------------------------------
	return new;
}

void *ua_realloc_wrapper(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ptr;
	void *new = ua_alloc(ua, sz);
	memcpy(new, ptr, old_sz);
	return new;
}

void ua_free_wrapper(UArena *ua, void *ptr)
{
	// NOTE: (isa): This will be called in the same places regular free will be,
	// which does not work for an arena, so its free function must be called
	// explicitly in tests
	(void)ptr;
	(void)ua;
}

void *malloc_wrapper(UArena *ua, size_t sz)
{
	(void)ua;
	return malloc(sz);
}

void *calloc_wrapper(UArena *ua, size_t sz)
{
	(void)ua;
	return calloc(1, sz);
}

void free_wrapper(UArena *ua, void *ptr)
{
	(void)ua;
	free(ptr);
}

void *realloc_wrapper(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ua;
	(void)old_sz;
	return realloc(ptr, sz);
}

static uint64_t malloc_total;
void *malloc_wrapper_timed(UArena *ua, size_t sz)
{
	(void)ua;
	uint64_t start = start_tsc_timing();
	//--------------------------------------
	void *ptr = malloc(sz);
	//--------------------------------------
	uint64_t end = end_tsc_timing();
	malloc_total += end - start;
	//--------------------------------------
	return ptr;
}

static uint64_t calloc_total;
void *calloc_wrapper_timed(UArena *ua, size_t sz)
{
	(void)ua;
	uint64_t start = start_tsc_timing();
	//--------------------------------------
	void *ptr = calloc(1, sz);
	//--------------------------------------
	uint64_t end = end_tsc_timing();
	calloc_total += end - start;
	//--------------------------------------
	return ptr;
}

static uint64_t free_total;
void free_wrapper_timed(UArena *ua, void *ptr)
{
	(void)ua;
	uint64_t start = start_tsc_timing();
	//--------------------------------------
	free(ptr);
	//--------------------------------------
	uint64_t end = end_tsc_timing();
	free_total += end - start;
}

static uint64_t realloc_total;
void *realloc_wrapper_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ua;
	(void)old_sz;
	uint64_t start = start_tsc_timing();
	//--------------------------------------
	void *new = realloc(ptr, sz);
	//--------------------------------------
	uint64_t end = end_tsc_timing();
	realloc_total += end - start;
	//--------------------------------------
	return new;
}

uint64_t get_and_clear_malloc_timing(void)
{
	uint64_t total = malloc_total;
	malloc_total = 0;
	return total;
}

uint64_t get_and_clear_calloc_timing(void)
{
	uint64_t total = calloc_total;
	calloc_total = 0;
	return total;
}

uint64_t get_and_clear_realloc_timing(void)
{
	uint64_t total = realloc_total;
	realloc_total = 0;
	return total;
}

uint64_t get_and_clear_free_timing(void)
{
	uint64_t total = free_total;
	free_total = 0;
	return total;
}

uint64_t get_and_clear_u_alloc_timing(void)
{
	uint64_t total = ua_alloc_total;
	ua_alloc_total = 0;
	return total;
}

uint64_t get_and_clear_u_zalloc_timing(void)
{
	uint64_t total = ua_zalloc_total;
	ua_zalloc_total = 0;
	return total;
}

uint64_t get_and_clear_u_falloc_timing(void)
{
	uint64_t total = ua_falloc_total;
	ua_falloc_total = 0;
	return total;
}

uint64_t get_and_clear_u_fzalloc_timing(void)
{
	uint64_t total = ua_fzalloc_total;
	ua_fzalloc_total = 0;
	return total;
}

uint64_t get_and_clear_u_realloc_timing(void)

{
	uint64_t total = ua_realloc_total;
	ua_realloc_total = 0;
	return total;
}
