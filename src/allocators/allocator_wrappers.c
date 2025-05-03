#include <src/lm.h>
LM_LOG_REGISTER(allocator_wrappers);

#include <src/metrics/timing.h>

#include "u_arena.h"
#include "allocator_wrappers.h"

#include <string.h>

static struct alloc_tstats tstats;
static struct alloc_tcoll tcoll = { 0, 0, NULL };

struct alloc_tstats *get_alloc_tstats(void)
{
	return &tstats;
}

void init_alloc_tstats(uint64_t cap, uint64_t *arr)
{
	tcoll.cur = 0;
	tcoll.cap = cap;
	tcoll.arr = arr;
}

struct alloc_tcoll *get_alloc_tcoll(void)
{
	return &tcoll;
}

static inline void add_timing(uint64_t t)
{
	tcoll.arr[tcoll.cur++] = t;
}

int write_alloc_timing_data_to_file(LmString filename,
				    enum allocation_type type)
{
	FILE *file = lm_open_file_by_name(filename, "wb");

	int res;
	if ((res = write_timing_by_type(type, file)) != 0)
		return res;

	if ((res = lm_write_bytes_to_file((uint8_t *)&tcoll.cur,
					  sizeof(tcoll.cur), file)) != 0)
		return res;

	if ((res = lm_write_bytes_to_file((uint8_t *)tcoll.arr,
					  tcoll.cur * sizeof(uint64_t),
					  file)) != 0)
		return res;

	LmLogInfo("Wrote timing stats and collection to %s", filename);
	return 0;
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

	tdata->tstats = at_stats;
	tdata->tcoll = tcoll;

	free(data);
}

void *ua_alloc_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING_LFENCE(alloc);
	//--------------------------------------
	void *ptr = ua_alloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING_LFENCE(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	tstats.ua_alloc_total_time += alloc_time;
	tstats.ua_alloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}

void *ua_zalloc_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING_LFENCE(alloc);
	//--------------------------------------
	void *ptr = ua_zalloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING_LFENCE(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	tstats.ua_zalloc_total_time += alloc_time;
	tstats.ua_zalloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}

void *ua_falloc_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING_LFENCE(alloc);
	//--------------------------------------
	void *ptr = ua_falloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING_LFENCE(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	tstats.ua_falloc_total_time += alloc_time;
	tstats.ua_falloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}

void *ua_fzalloc_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING_LFENCE(alloc);
	//--------------------------------------
	void *ptr = ua_fzalloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING_LFENCE(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	tstats.ua_fzalloc_total_time += alloc_time;
	tstats.ua_fzalloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}

void *ua_realloc_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ptr;
	START_TSC_TIMING_LFENCE(alloc);
	//--------------------------------------
	void *new = ua_alloc(ua, sz);
	memcpy(new, ptr, old_sz);
	//--------------------------------------
	END_TSC_TIMING_LFENCE(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	tstats.ua_realloc_total_time += alloc_time;
	tstats.ua_realloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return new;
}

void *malloc_timed(UArena *ua, size_t sz)
{
	(void)ua;
	START_TSC_TIMING_LFENCE(alloc);
	//--------------------------------------
	void *ptr = malloc(sz);
	//--------------------------------------
	END_TSC_TIMING_LFENCE(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	tstats.malloc_total_time += alloc_time;
	tstats.malloc_total_iter += 1;
	add_timing(alloc_time);
	tcoll.arr[tcoll.cur++] = alloc_time;
	//--------------------------------------
	return ptr;
}

void *calloc_timed(UArena *ua, size_t sz)
{
	(void)ua;
	START_TSC_TIMING_LFENCE(alloc);
	//--------------------------------------
	void *ptr = calloc(1, sz);
	//--------------------------------------
	END_TSC_TIMING_LFENCE(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
	tstats.calloc_total_time += alloc_time;
	tstats.calloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}

void free_timed(UArena *ua, void *ptr)
{
	(void)ua;
	START_TSC_TIMING_LFENCE(free);
	//--------------------------------------
	free(ptr);
	//--------------------------------------
	END_TSC_TIMING_LFENCE(free);
	tstats.free_total_time += free_end - free_start;
	tstats.free_total_iter += 1;
}

void *realloc_timed(UArena *ua, void *ptr, size_t old_sz, size_t sz)
{
	(void)ua;
	(void)old_sz;
	START_TSC_TIMING_LFENCE(realloc);
	//--------------------------------------
	void *new = realloc(ptr, sz);
	//--------------------------------------
	END_TSC_TIMING_LFENCE(realloc);
	uint64_t alloc_time = realloc_end - realloc_start;
	tstats.realloc_total_time += alloc_time;
	tstats.realloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return new;
}
