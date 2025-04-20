#include <src/lm.h>
LM_LOG_REGISTER(allocator_wrappers);

#include <src/metrics/timing.h>

#include "u_arena.h"
#include "allocator_wrappers.h"

static long long u_alloc_total, u_alloc_start, u_alloc_end;
void *ua_alloc_wrapper_timed(UArena *ua, size_t sz)
{
	u_alloc_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *ptr = ua_alloc(ua, sz);
	//--------------------------------------
	u_alloc_end = lm_get_time_stamp(PROC_CPUTIME);
	u_alloc_total += (u_alloc_end - u_alloc_start);
	//--------------------------------------
	return ptr;
}

static long long u_zalloc_total, u_zalloc_start, u_zalloc_end;
void *ua_zalloc_wrapper_timed(UArena *ua, size_t sz)
{
	u_zalloc_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *ptr = ua_zalloc(ua, sz);
	//--------------------------------------
	u_zalloc_end = lm_get_time_stamp(PROC_CPUTIME);
	u_zalloc_total += (u_zalloc_end - u_zalloc_start);
	//--------------------------------------
	return ptr;
}

static long long u_falloc_total, u_falloc_start, u_falloc_end;
void *ua_falloc_wrapper_timed(UArena *ua, size_t sz)
{
	u_falloc_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *ptr = ua_falloc(ua, sz);
	//--------------------------------------
	u_falloc_end = lm_get_time_stamp(PROC_CPUTIME);
	u_falloc_total += (u_falloc_end - u_falloc_start);
	//--------------------------------------
	return ptr;
}

static long long u_fzalloc_total, u_fzalloc_start, u_fzalloc_end;
void *ua_fzalloc_wrapper_timed(UArena *ua, size_t sz)
{
	u_fzalloc_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *ptr = ua_fzalloc(ua, sz);
	//--------------------------------------
	u_fzalloc_end = lm_get_time_stamp(PROC_CPUTIME);
	u_fzalloc_total += (u_fzalloc_end - u_fzalloc_start);
	//--------------------------------------
	return ptr;
}

// TODO: (isa): This is *not* how you're supposed to use an arena, but it allows
// us to run tests for malloc-like APIs
static long long u_realloc_total, u_realloc_start, u_realloc_end;
void *ua_realloc_wrapper_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ptr;
	u_realloc_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *new = ua_alloc(ua, sz);
	memcpy(new, ptr, old_sz);
	//--------------------------------------
	u_realloc_end = lm_get_time_stamp(PROC_CPUTIME);
	u_realloc_total += (u_realloc_end - u_realloc_start);
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

static long long malloc_total, malloc_start, malloc_end;
void *malloc_wrapper_timed(UArena *ua, size_t sz)
{
	(void)ua;
	malloc_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *ptr = malloc(sz);
	//--------------------------------------
	malloc_end = lm_get_time_stamp(PROC_CPUTIME);
	malloc_total += (malloc_end - malloc_start);
	//--------------------------------------
	return ptr;
}

static long long calloc_total, calloc_start, calloc_end;
void *calloc_wrapper_timed(UArena *ua, size_t sz)
{
	(void)ua;
	calloc_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *ptr = calloc(1, sz);
	//--------------------------------------
	calloc_end = lm_get_time_stamp(PROC_CPUTIME);
	calloc_total += (calloc_end - calloc_start);
	//--------------------------------------
	return ptr;
}

static long long free_total, free_start, free_end;
void free_wrapper_timed(UArena *ua, void *ptr)
{
	(void)ua;
	free_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	free(ptr);
	//--------------------------------------
	free_end = lm_get_time_stamp(PROC_CPUTIME);
	free_total += (free_end - free_start);
}

static long long realloc_total, realloc_start, realloc_end;
void *realloc_wrapper_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ua;
	(void)old_sz;
	realloc_start = lm_get_time_stamp(PROC_CPUTIME);
	//--------------------------------------
	void *new = realloc(ptr, sz);
	//--------------------------------------
	realloc_end = lm_get_time_stamp(PROC_CPUTIME);
	realloc_total += (realloc_end - realloc_start);
	//--------------------------------------
	return new;
}

long long get_and_clear_malloc_timing(void)
{
	long long total = malloc_total;
	malloc_total = 0;
	return total;
}

long long get_and_clear_calloc_timing(void)
{
	long long total = calloc_total;
	calloc_total = 0;
	return total;
}

long long get_and_clear_realloc_timing(void)
{
	long long total = realloc_total;
	realloc_total = 0;
	return total;
}

long long get_and_clear_free_timing(void)
{
	long long total = free_total;
	free_total = 0;
	return total;
}

long long get_and_clear_u_alloc_timing(void)
{
	long long total = u_alloc_total;
	u_alloc_total = 0;
	return total;
}

long long get_and_clear_u_zalloc_timing(void)
{
	long long total = u_zalloc_total;
	u_zalloc_total = 0;
	return total;
}

long long get_and_clear_u_falloc_timing(void)
{
	long long total = u_falloc_total;
	u_falloc_total = 0;
	return total;
}

long long get_and_clear_u_fzalloc_timing(void)
{
	long long total = u_fzalloc_total;
	u_fzalloc_total = 0;
	return total;
}

long long get_and_clear_u_realloc_timing(void)
{
	long long total = u_realloc_total;
	u_realloc_total = 0;
	return total;
}
