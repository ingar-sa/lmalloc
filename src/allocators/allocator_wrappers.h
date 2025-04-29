#ifndef ALLOCATOR_WRAPPERS_H
#define ALLOCATOR_WRAPPERS_H

#include <src/lm.h>

#include <src/metrics/timing.h>

#include "u_arena.h"

struct alloc_timing_collection {
	uint64_t cap;
	uint64_t idx;
	uint64_t *arr;
};

// NOTE: (isa): (ingar): Made by Claude
enum allocation_type {
	MALLOC,
	CALLOC,
	REALLOC,
	FREE,
	UA_ALLOC,
	UA_ZALLOC,
	UA_FALLOC,
	UA_FZALLOC,
	UA_REALLOC
};

// NOTE: (isa): Mayde by Claude
static inline const char *alloct_string(enum allocation_type type)
{
	switch (type) {
	case MALLOC:
		return "malloc";
	case CALLOC:
		return "calloc";
	case REALLOC:
		return "realloc";
	case FREE:
		return "free";
	case UA_ALLOC:
		return "ua_alloc";
	case UA_ZALLOC:
		return "ua_zalloc";
	case UA_FALLOC:
		return "ua_falloc";
	case UA_FZALLOC:
		return "ua_fzalloc";
	case UA_REALLOC:
		return "ua_realloc";
	default:
		return "unknown";
	}
}

// NOTE: (isa): (ingar): Struct made by Claude
struct alloc_timing_stats {
	uint64_t malloc_total_time;
	uint64_t malloc_total_iter;

	uint64_t calloc_total_time;
	uint64_t calloc_total_iter;

	uint64_t realloc_total_time;
	uint64_t realloc_total_iter;

	uint64_t free_total_time;
	uint64_t free_total_iter;

	uint64_t ua_alloc_total_time;
	uint64_t ua_alloc_total_iter;

	uint64_t ua_zalloc_total_time;
	uint64_t ua_zalloc_total_iter;

	uint64_t ua_falloc_total_time;
	uint64_t ua_falloc_total_iter;

	uint64_t ua_fzalloc_total_time;
	uint64_t ua_fzalloc_total_iter;

	uint64_t ua_realloc_total_time;
	uint64_t ua_realloc_total_iter;
};

struct alloc_timing_data {
	struct alloc_timing_stats *stats;
	struct alloc_timing_collection *tcoll;
};

typedef void *(*alloc_fn_t)(UArena *ua, size_t sz);
typedef void (*free_fn_t)(UArena *ua, void *ptr);
typedef void *(*realloc_fn_t)(UArena *ua, void *ptr, size_t old_sz, size_t sz);

struct alloc_timing_stats *get_alloc_stats(void);
void provide_alloc_timing_collection_arr(uint64_t cap, uint64_t *arr);
void clear_wrapper_alloc_timing_collection(void);
struct alloc_timing_collection *get_wrapper_timings(void);
void get_allocation_stats(struct alloc_timing_stats *stats,
			  enum allocation_type type, uint64_t *timing,
			  uint64_t *iterations);

void write_timing_data_to_file(LmString filename);
void read_timing_data_from_file(const char *filename,
				struct alloc_timing_data *tdata, UArena *ua);

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

void log_allocation_timing_avg(enum allocation_type type,
			       struct alloc_timing_stats *stats,
			       const char *description, enum time_stamp_fmt fmt,
			       bool log_raw, enum lm_log_level lvl,
			       lm_log_module *module);

void log_allocation_timing(enum allocation_type type,
			   struct alloc_timing_stats *stats,
			   const char *description, enum time_stamp_fmt fmt,
			   bool log_raw, enum lm_log_level lvl,
			   lm_log_module *module);

#endif
