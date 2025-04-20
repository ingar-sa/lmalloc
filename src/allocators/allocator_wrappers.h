#ifndef ALLOCATOR_WRAPPERS_H
#define ALLOCATOR_WRAPPERS_H

#include <src/lm.h>

#include "u_arena.h"

typedef void *(*alloc_fn_t)(UArena *ua, size_t sz);
typedef void (*free_fn_t)(UArena *ua, void *ptr);
typedef void *(*realloc_fn_t)(UArena *ua, void *ptr, size_t old_sz, size_t sz);

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

long long get_and_clear_malloc_timing(void);
long long get_and_clear_calloc_timing(void);
long long get_and_clear_realloc_timing(void);
long long get_and_clear_free_timing(void);
long long get_and_clear_u_alloc_timing(void);
long long get_and_clear_u_zalloc_timing(void);
long long get_and_clear_u_falloc_timing(void);
long long get_and_clear_u_fzalloc_timing(void);
long long get_and_clear_u_realloc_timing(void);

#define TIME_U_ARENA 1
#define TIME_MALLOC 1

#if TIME_U_ARENA == 1
static const alloc_fn_t ua__alloc_functions[] = { ua_alloc_wrapper_timed,
						  ua_zalloc_wrapper_timed,
						  ua_falloc_wrapper_timed,
						  ua_fzalloc_wrapper_timed };
static const free_fn_t ua__free_functions[] = { ua_free_wrapper };
static const realloc_fn_t ua__realloc_functions[] = { ua_realloc_wrapper_timed };
#else
static const alloc_fn_t ua__alloc_functions[] = { ua_alloc, ua_zalloc,
						  ua_falloc, ua_fzalloc };
static const free_fn_t ua_free_functions[] = { ua_free_wrapper };
static const realloc_fn_t ua_realloc_functions[] = { ua_realloc_wrapper };
#endif

#if TIME_MALLOC == 1
static const alloc_fn_t malloc_and_fam[] = { malloc_wrapper_timed,
					     calloc_wrapper_timed };
static const free_fn_t free_functions[] = { free_wrapper_timed };
static const realloc_fn_t realloc_functions[] = { realloc_wrapper_timed };
#else
static const alloc_fn_t malloc_and_fam[] = { malloc_wrapper, calloc_wrapper };
static const free_fn_t free_functions[] = { free_wrapper };
static const realloc_fn_t realloc_functions[] = { realloc_wrapper };
#endif

static const char *ua_alloc_function_names[] = { "alloc", "zalloc", "falloc",
						 "fzalloc" };

static const char *malloc_and_fam_names[] = { "malloc", "calloc" };
#endif
