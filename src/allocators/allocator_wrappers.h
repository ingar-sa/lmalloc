#ifndef ALLOCATOR_WRAPPERS_H
#define ALLOCATOR_WRAPPERS_H

#include <src/lm.h>

#include <src/metrics/timing.h>

#include "u_arena.h"

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

struct alloc_tcoll {
	uint64_t cap;
	uint64_t cur;
	uint64_t *arr;
};

struct alloc_tstats {
	uint64_t total_time;
	uint64_t iter;
};

struct alloc_timing_data {
	struct alloc_tstats *tstats;
	struct alloc_tcoll *tcoll;
};

typedef void *(*alloc_fn_t)(UArena *ua, size_t sz);
typedef void (*free_fn_t)(UArena *ua, void *ptr);
typedef void *(*realloc_fn_t)(UArena *ua, void *ptr, size_t old_sz, size_t sz);

struct alloc_tstats *get_alloc_stats(void);
void init_alloc_tcoll(uint64_t cap, uint64_t *arr);
struct alloc_tcoll *get_alloc_tcoll(void);

int write_alloc_timing_data_to_file(LmString filename,
				    enum allocation_type type);
void read_timing_data_from_file(const char *filename,
				struct alloc_timing_data *tdata, UArena *ua);

void *ua_alloc_timed(UArena *ua, size_t sz);
void *ua_zalloc_timed(UArena *ua, size_t sz);
void *ua_falloc_timed(UArena *ua, size_t sz);
void *ua_fzalloc_timed(UArena *ua, size_t sz);
void *ua_realloc_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz);

void *malloc_timed(UArena *ua, size_t sz);
void *calloc_timed(UArena *ua, size_t sz);
void free_timed(UArena *ua, void *ptr);
void *realloc_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz);

#endif
