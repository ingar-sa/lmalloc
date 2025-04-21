#include <src/lm.h>
LM_LOG_REGISTER(allocator_wrappers);

#include <src/metrics/timing.h>

#include "u_arena.h"
#include "allocator_wrappers.h"

static uint64_t ua_alloc_total_time, ua_alloc_total_iter;
void *ua_alloc_wrapper_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = ua_alloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	ua_alloc_total_time += alloc_end - alloc_start;
	ua_alloc_total_iter += 1;
	//--------------------------------------
	return ptr;
}

static uint64_t ua_zalloc_total_time, ua_zalloc_total_iter;
void *ua_zalloc_wrapper_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = ua_zalloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	ua_zalloc_total_time += alloc_end - alloc_start;
	ua_zalloc_total_iter += 1;
	//--------------------------------------
	return ptr;
}

static uint64_t ua_falloc_total_time, ua_falloc_total_iter;
void *ua_falloc_wrapper_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = ua_falloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	ua_falloc_total_time += alloc_end - alloc_start;
	ua_falloc_total_iter += 1;
	//--------------------------------------
	return ptr;
}

static uint64_t ua_fzalloc_total_time, ua_fzalloc_total_iter;
void *ua_fzalloc_wrapper_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = ua_fzalloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	ua_fzalloc_total_time += alloc_end - alloc_start;
	ua_fzalloc_total_iter += 1;
	//--------------------------------------
	return ptr;
}

static uint64_t ua_realloc_total_time, ua_realloc_total_iter;
void *ua_realloc_wrapper_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ptr;
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *new = ua_alloc(ua, sz);
	memcpy(new, ptr, old_sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	ua_realloc_total_time += alloc_end - alloc_start;
	ua_realloc_total_iter += 1;
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

static uint64_t malloc_total_time, malloc_total_iter;
void *malloc_wrapper_timed(UArena *ua, size_t sz)
{
	(void)ua;
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = malloc(sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	malloc_total_time += alloc_end - alloc_start;
	malloc_total_iter += 1;
	//--------------------------------------
	return ptr;
}

static uint64_t calloc_total_time, calloc_total_iter;
void *calloc_wrapper_timed(UArena *ua, size_t sz)
{
	(void)ua;
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = calloc(1, sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	calloc_total_time += alloc_end - alloc_start;
	calloc_total_iter += 1;
	//--------------------------------------
	return ptr;
}

static uint64_t free_total_time, free_total_iter;
void free_wrapper_timed(UArena *ua, void *ptr)
{
	(void)ua;
	START_TSC_TIMING(free);
	//--------------------------------------
	free(ptr);
	//--------------------------------------
	END_TSC_TIMING(free);
	free_total_time += free_end - free_start;
	free_total_iter += 1;
}

static uint64_t realloc_total_time, realloc_total_iter;
void *realloc_wrapper_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ua;
	(void)old_sz;
	START_TSC_TIMING(realloc);
	//--------------------------------------
	void *new = realloc(ptr, sz);
	//--------------------------------------
	END_TSC_TIMING(realloc);
	realloc_total_time += realloc_end - realloc_start;
	realloc_total_iter += 1;
	//--------------------------------------
	return new;
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

uint64_t get_and_clear_malloc_timing(void)
{
	uint64_t total = malloc_total_time;
	malloc_total_time = 0;
	return total;
}

uint64_t get_and_clear_malloc_iterations(void)
{
	uint64_t total = malloc_total_iter;
	malloc_total_iter = 0;
	return total;
}

uint64_t get_and_clear_calloc_timing(void)
{
	uint64_t total = calloc_total_time;
	calloc_total_time = 0;
	return total;
}

uint64_t get_and_clear_calloc_iterations(void)
{
	uint64_t total = calloc_total_iter;
	calloc_total_iter = 0;
	return total;
}

uint64_t get_and_clear_realloc_timing(void)
{
	uint64_t total = realloc_total_time;
	realloc_total_time = 0;
	return total;
}

uint64_t get_and_clear_realloc_iterations(void)
{
	uint64_t total = realloc_total_iter;
	realloc_total_iter = 0;
	return total;
}

uint64_t get_and_clear_free_timing(void)
{
	uint64_t total = free_total_time;
	free_total_time = 0;
	return total;
}

uint64_t get_and_clear_free_iterations(void)
{
	uint64_t total = free_total_iter;
	free_total_iter = 0;
	return total;
}

uint64_t get_and_clear_ua_alloc_timing(void)
{
	uint64_t total = ua_alloc_total_time;
	ua_alloc_total_time = 0;
	return total;
}

uint64_t get_and_clear_ua_alloc_iterations(void)
{
	uint64_t total = ua_alloc_total_iter;
	ua_alloc_total_iter = 0;
	return total;
}

uint64_t get_and_clear_ua_zalloc_timing(void)
{
	uint64_t total = ua_zalloc_total_time;
	ua_zalloc_total_time = 0;
	return total;
}

uint64_t get_and_clear_ua_zalloc_iterations(void)
{
	uint64_t total = ua_zalloc_total_iter;
	ua_zalloc_total_iter = 0;
	return total;
}

uint64_t get_and_clear_ua_falloc_timing(void)
{
	uint64_t total = ua_falloc_total_time;
	ua_falloc_total_time = 0;
	return total;
}

uint64_t get_and_clear_ua_falloc_iterations(void)
{
	uint64_t total = ua_falloc_total_iter;
	ua_falloc_total_iter = 0;
	return total;
}

uint64_t get_and_clear_ua_fzalloc_timing(void)
{
	uint64_t total = ua_fzalloc_total_time;
	ua_fzalloc_total_time = 0;
	return total;
}

uint64_t get_and_clear_ua_fzalloc_iterations(void)
{
	uint64_t total = ua_fzalloc_total_iter;
	ua_fzalloc_total_iter = 0;
	return total;
}

uint64_t get_and_clear_ua_realloc_timing(void)
{
	uint64_t total = ua_realloc_total_time;
	ua_realloc_total_time = 0;
	return total;
}

uint64_t get_and_clear_ua_realloc_iterations(void)
{
	uint64_t total = ua_realloc_total_iter;
	ua_realloc_total_iter = 0;
	return total;
}
