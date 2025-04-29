#include <src/lm.h>
LM_LOG_REGISTER(allocator_wrappers);

#include <src/metrics/timing.h>

#include "u_arena.h"
#include "allocator_wrappers.h"

#include <string.h>

static struct alloc_timing_stats timing_stats;
struct alloc_timing_stats *get_alloc_stats(void)
{
	return &timing_stats;
}

static uint64_t timings_zii[1];
static struct alloc_timing_collection timings = { 0, 0, timings_zii };

void provide_alloc_timing_collection_arr(uint64_t cap, uint64_t *arr)
{
	timings.cap = cap;
	timings.arr = arr;
}

void clear_wrapper_alloc_timing_collection(void)
{
	timings.cap = 0;
	timings.idx = 0;
	timings.arr = timings_zii;
}

struct alloc_timing_collection *get_wrapper_timings(void)
{
	return &timings;
}

static void add_timing(uint64_t t)
{
	if (LM_LIKELY(timings.arr != timings_zii)) {
		timings.arr[timings.idx++] = t;
	}
}

void write_timing_data_to_file(LmString filename)
{
	FILE *file = lm_open_file_by_name(filename, "w");

	int res;
	size_t tstats_sz = sizeof(timing_stats);
	if ((res = lm_write_bytes_to_file((const uint8_t *)&tstats_sz,
					  sizeof(tstats_sz), file)) != 0)
		return;

	if ((res = lm_write_bytes_to_file((const uint8_t *)(&timing_stats),
					  sizeof(timing_stats), file)) != 0)
		return;

	if ((res = lm_write_bytes_to_file((const uint8_t *)&timings.idx,
					  sizeof(timings.idx), file)) != 0)
		return;

	if ((res = lm_write_bytes_to_file((const uint8_t *)timings.arr,
					  timings.idx * sizeof(uint64_t),
					  file)) != 0)
		return;

	LmLogInfo("Wrote timing stats and collection to %s", filename);
}

void read_timing_data_from_file(const char *filename,
				struct alloc_timing_data *tdata, UArena *ua)
{
	size_t file_sz;
	uint8_t *data = lm_load_file_into_memory(filename, &file_sz, NULL);
	if (!data)
		return;

	struct alloc_timing_stats *at_stats =
		UaPushStruct(ua, struct alloc_timing_stats);
	struct alloc_timing_collection *tcoll =
		UaPushStruct(ua, struct alloc_timing_collection);

	uintptr_t ptr = (uintptr_t)data;
	tcoll->cap = *((uint64_t *)(ptr + sizeof(struct alloc_timing_stats)));
	tcoll->idx = tcoll->cap;
	tcoll->arr = UaPushArray(ua, uint64_t, tcoll->cap);

	memcpy(at_stats, data, sizeof(*at_stats));
	uint8_t *tcoll_data =
		(uint8_t *)(data + sizeof(*at_stats) + sizeof(uint64_t));
	memcpy(tcoll, tcoll_data, tcoll->cap);

	tdata->stats = at_stats;
	tdata->tcoll = tcoll;

	free(data);
}

void *ua_alloc_wrapper_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = ua_alloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	timing_stats.ua_alloc_total_time += alloc_time;
	timing_stats.ua_alloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}

void *ua_zalloc_wrapper_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = ua_zalloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	timing_stats.ua_zalloc_total_time += alloc_time;
	timing_stats.ua_zalloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}

void *ua_falloc_wrapper_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = ua_falloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	timing_stats.ua_falloc_total_time += alloc_time;
	timing_stats.ua_falloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}

void *ua_fzalloc_wrapper_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = ua_fzalloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	timing_stats.ua_fzalloc_total_time += alloc_time;
	timing_stats.ua_fzalloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}

void *ua_realloc_wrapper_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ptr;
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *new = ua_alloc(ua, sz);
	memcpy(new, ptr, old_sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	timing_stats.ua_realloc_total_time += alloc_time;
	timing_stats.ua_realloc_total_iter += 1;
	add_timing(alloc_time);
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

void *malloc_wrapper_timed(UArena *ua, size_t sz)
{
	(void)ua;
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = malloc(sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	timing_stats.malloc_total_time += alloc_time;
	timing_stats.malloc_total_iter += 1;
	add_timing(alloc_time);
	timings.arr[timings.idx++] = alloc_time;
	//--------------------------------------
	return ptr;
}

void *calloc_wrapper_timed(UArena *ua, size_t sz)
{
	(void)ua;
	START_TSC_TIMING(alloc);
	//--------------------------------------
	void *ptr = calloc(1, sz);
	//--------------------------------------
	END_TSC_TIMING(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	timing_stats.calloc_total_time += alloc_time;
	timing_stats.calloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}

void free_wrapper_timed(UArena *ua, void *ptr)
{
	(void)ua;
	START_TSC_TIMING(free);
	//--------------------------------------
	free(ptr);
	//--------------------------------------
	END_TSC_TIMING(free);
	timing_stats.free_total_time += free_end - free_start;
	timing_stats.free_total_iter += 1;
}

void *realloc_wrapper_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ua;
	(void)old_sz;
	START_TSC_TIMING(realloc);
	//--------------------------------------
	void *new = realloc(ptr, sz);
	//--------------------------------------
	END_TSC_TIMING(realloc);
	uint64_t alloc_time = realloc_end - realloc_start;
	timing_stats.realloc_total_time += alloc_time;
	timing_stats.realloc_total_iter += 1;
	add_timing(alloc_time);
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

// NOTE: (isa): (ingar): Written by Claude
void log_allocation_timing(enum allocation_type type,
			   struct alloc_timing_stats *stats,
			   const char *description, enum time_stamp_fmt fmt,
			   bool log_raw, enum lm_log_level lvl,
			   lm_log_module *module)
{
	switch (type) {
	case MALLOC:
		lm_log_tsc_timing(stats->malloc_total_time, description, fmt,
				  log_raw, lvl, module);
		break;

	case CALLOC:
		lm_log_tsc_timing(stats->malloc_total_time, description, fmt,
				  log_raw, lvl, module);
		break;

	case REALLOC:
		lm_log_tsc_timing(stats->malloc_total_time, description, fmt,
				  log_raw, lvl, module);
		break;

	case FREE:
		lm_log_tsc_timing(stats->malloc_total_time, description, fmt,
				  log_raw, lvl, module);
		break;

	case UA_ALLOC:
		lm_log_tsc_timing(stats->malloc_total_time, description, fmt,
				  log_raw, lvl, module);
		break;

	case UA_ZALLOC:
		lm_log_tsc_timing(stats->malloc_total_time, description, fmt,
				  log_raw, lvl, module);
		break;

	case UA_FALLOC:
		lm_log_tsc_timing(stats->malloc_total_time, description, fmt,
				  log_raw, lvl, module);
		break;

	case UA_FZALLOC:
		lm_log_tsc_timing(stats->malloc_total_time, description, fmt,
				  log_raw, lvl, module);
		break;

	case UA_REALLOC:
		lm_log_tsc_timing(stats->malloc_total_time, description, fmt,
				  log_raw, lvl, module);
		break;

	default:
		LmLogWarning("Unknown allocation type");
		break;
	}
}

// NOTE: (isa): (ingar): Written by Claude
void log_allocation_timing_avg(enum allocation_type type,
			       struct alloc_timing_stats *stats,
			       const char *description, enum time_stamp_fmt fmt,
			       bool log_raw, enum lm_log_level lvl,
			       lm_log_module *module)
{
	switch (type) {
	case MALLOC:
		lm_log_tsc_timing_avg(stats->malloc_total_time,
				      stats->malloc_total_iter, description,
				      fmt, log_raw, lvl, module);
		break;

	case CALLOC:
		lm_log_tsc_timing_avg(stats->calloc_total_time,
				      stats->calloc_total_iter, description,
				      fmt, log_raw, lvl, module);
		break;

	case REALLOC:
		lm_log_tsc_timing_avg(stats->realloc_total_time,
				      stats->realloc_total_iter, description,
				      fmt, log_raw, lvl, module);
		break;

	case FREE:
		lm_log_tsc_timing_avg(stats->free_total_time,
				      stats->free_total_iter, description, fmt,
				      log_raw, lvl, module);
		break;

	case UA_ALLOC:
		lm_log_tsc_timing_avg(stats->ua_alloc_total_time,
				      stats->ua_alloc_total_iter, description,
				      fmt, log_raw, lvl, module);
		break;

	case UA_ZALLOC:
		lm_log_tsc_timing_avg(stats->ua_zalloc_total_time,
				      stats->ua_zalloc_total_iter, description,
				      fmt, log_raw, lvl, module);
		break;

	case UA_FALLOC:
		lm_log_tsc_timing_avg(stats->ua_falloc_total_time,
				      stats->ua_falloc_total_iter, description,
				      fmt, log_raw, lvl, module);
		break;

	case UA_FZALLOC:
		lm_log_tsc_timing_avg(stats->ua_fzalloc_total_time,
				      stats->ua_fzalloc_total_iter, description,
				      fmt, log_raw, lvl, module);
		break;

	case UA_REALLOC:
		lm_log_tsc_timing_avg(stats->ua_realloc_total_time,
				      stats->ua_realloc_total_iter, description,
				      fmt, log_raw, lvl, module);
		break;

	default:
		LmLogWarning("Unknown allocation type\n");
		break;
	}
}
