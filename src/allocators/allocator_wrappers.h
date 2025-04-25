#ifndef ALLOCATOR_WRAPPERS_H
#define ALLOCATOR_WRAPPERS_H

#include <src/lm.h>

#include "u_arena.h"

struct timing_collection {
	uint64_t cap;
	uint64_t idx;
	uint64_t *arr;
};

typedef void *(*alloc_fn_t)(UArena *ua, size_t sz);
typedef void (*free_fn_t)(UArena *ua, void *ptr);
typedef void *(*realloc_fn_t)(UArena *ua, void *ptr, size_t old_sz, size_t sz);

void provide_timing_collection_arr(uint64_t cap, uint64_t *arr);
void clear_wrapper_timing_collection(void);
struct timing_collection *get_wrapper_timings(void);

void *ua_alloc_wrapper_timed(UArena *ua, size_t sz);
void *ua_zalloc_wrapper_timed(UArena *ua, size_t sz);
void *ua_falloc_wrapper_timed(UArena *ua, size_t sz);
void *ua_fzalloc_wrapper_timed(UArena *ua, size_t sz);
void *ua_realloc_wrapper_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz);

void ua_free_wrapper(UArena *ua, void *ptr);
void *ua_realloc_wrapper(UArena *ua, void *ptr, size_t old_sz, size_t sz);

void *malloc_wrapper(UArena *ua, size_t sz);
void *calloc_wrapper(UArena *ua, size_t sz);
void free_wrapper(UArena *ua, void *ptr);
void *realloc_wrapper(UArena *ua, void *ptr, size_t old_sz, size_t sz);

void *malloc_wrapper_timed(UArena *ua, size_t sz);
void *calloc_wrapper_timed(UArena *ua, size_t sz);
void free_wrapper_timed(UArena *ua, void *ptr);
void *realloc_wrapper_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz);

uint64_t get_and_clear_malloc_timing(void);
uint64_t get_and_clear_calloc_timing(void);
uint64_t get_and_clear_realloc_timing(void);
uint64_t get_and_clear_free_timing(void);
uint64_t get_and_clear_ua_alloc_timing(void);
uint64_t get_and_clear_ua_zalloc_timing(void);
uint64_t get_and_clear_ua_falloc_timing(void);
uint64_t get_and_clear_ua_fzalloc_timing(void);
uint64_t get_and_clear_ua_realloc_timing(void);

uint64_t get_and_clear_malloc_iterations(void);
uint64_t get_and_clear_calloc_iterations(void);
uint64_t get_and_clear_realloc_iterations(void);
uint64_t get_and_clear_free_iterations(void);
uint64_t get_and_clear_ua_alloc_iterations(void);
uint64_t get_and_clear_ua_zalloc_iterations(void);
uint64_t get_and_clear_ua_falloc_iterations(void);
uint64_t get_and_clear_ua_fzalloc_iterations(void);
uint64_t get_and_clear_ua_realloc_iterations(void);

#endif
