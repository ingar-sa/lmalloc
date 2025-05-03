#ifndef ALLOCATOR_WRAPPERS_H
#define ALLOCATOR_WRAPPERS_H

#include <src/lm.h>
#include <src/metrics/timing.h>

#include "u_arena.h"
#include "karena.h"

// NOTE: (isa): (ingar): Made by Claude
enum alloc_type {
	MALLOC,
	CALLOC,
	REALLOC,
	FREE,
	KA_ALLOC,
	UA_ALLOC,
	UA_ZALLOC,
	UA_FALLOC,
	UA_FZALLOC,
	UA_REALLOC,
	UNKNOWN
};

// NOTE: (isa): Made by Claude
static inline const char *alloct_string(enum alloc_type type)
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
	case KA_ALLOC:
		return "ka_alloc";
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
	uint64_t total_tsc;
	uint64_t iter;
};

struct alloc_timing_data {
	struct alloc_tstats *tstats;
	struct alloc_tcoll *tcoll;
};

typedef void *(*alloc_fn_t)(UArena *ua, KArena *ka, size_t sz);
typedef void (*free_fn_t)(UArena *ua, KArena *ka, void *ptr);
typedef void *(*realloc_fn_t)(UArena *ua, KArena *ka, void *ptr, size_t old_sz,
			      size_t sz);

struct alloc_tstats *get_alloc_tstats(void);
void init_alloc_tcoll(uint64_t cap, uint64_t *arr);
void init_alloc_tcoll_dynamic(size_t cap);
struct alloc_tcoll *get_alloc_tcoll(void);

int write_alloc_timing_data_to_file(LmString filename, enum alloc_type type);

void *ka_alloc_timed(UArena *ua, KArena *ka, size_t sz);

void *ua_alloc_timed(UArena *ua, KArena *ka, size_t sz);
void *ua_zalloc_timed(UArena *ua, KArena *ka, size_t sz);
void *ua_falloc_timed(UArena *ua, KArena *ka, size_t sz);
void *ua_fzalloc_timed(UArena *ua, KArena *ka, size_t sz);
void *ua_realloc_timed(UArena *ua, KArena *ka, void *ptr, size_t old_sz,
		       size_t sz);

void *malloc_timed(UArena *ua, KArena *ka, size_t sz);
void *calloc_timed(UArena *ua, KArena *ka, size_t sz);
void *realloc_timed(UArena *ua, KArena *ka, void *ptr, size_t old_sz,
		    size_t sz);
void free_timed(UArena *ua, void *ptr);

static enum alloc_type get_alloc_type(alloc_fn_t alloc_fn)
{
	enum alloc_type type = UNKNOWN;
	if (alloc_fn == ka_alloc_timed)
		type = KA_ALLOC;
	else if (alloc_fn == ua_alloc_timed)
		type = UA_ALLOC;
	else if (alloc_fn == ua_zalloc_timed)
		type = UA_ZALLOC;
	else if (alloc_fn == ua_falloc_timed)
		type = UA_FALLOC;
	else if (alloc_fn == ua_fzalloc_timed)
		type = UA_FZALLOC;
	else if (alloc_fn == malloc_timed)
		type = MALLOC;
	else if (alloc_fn == calloc_timed)
		type = CALLOC;

	return type;
}

//void read_timing_data_from_file(const char *filename,
//				struct alloc_timing_data *tdata, UArena *ua);

#endif
