#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define START_TSC_TIMING_LFENCE(name)                              \
	uint64_t name##_start;                                     \
	do {                                                       \
		uint32_t low, high;                                \
		__asm__ volatile("lfence");                        \
		__asm__ volatile("rdtsc" : "=a"(low), "=d"(high)); \
		__asm__ volatile("lfence");                        \
		name##_start = ((uint64_t)high << 32) | low;       \
	} while (0)

#define END_TSC_TIMING_LFENCE(name)                                \
	uint64_t name##_end;                                       \
	do {                                                       \
		uint32_t low, high;                                \
		__asm__ volatile("lfence");                        \
		__asm__ volatile("rdtsc" : "=a"(low), "=d"(high)); \
		__asm__ volatile("lfence");                        \
		name##_end = ((uint64_t)high << 32) | low;         \
	} while (0)

#define LM_LIKELY(x) __builtin_expect(!!(x), 1)
#define LmPow2AlignUp(value, alignment) ((alignment - 1) & (-value))

typedef struct {
	uint_least64_t flags;
	size_t alignment;
	size_t cap;
	size_t cur;
	uint8_t *mem;
} UArena;

struct alloc_timing_collection {
	uint64_t cap;
	uint64_t idx;
	uint64_t *arr;
};

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

static uint64_t timings_zii[1];
static struct alloc_timing_collection timings = { 0, 0, timings_zii };
static struct alloc_timing_stats timing_stats;

struct alloc_timing_stats *get_alloc_stats(void)
{
	return &timing_stats;
}

static inline void add_timing(uint64_t t)
{
	if (LM_LIKELY(timings.arr != timings_zii)) {
		timings.arr[timings.idx++] = t;
	}
}

inline void *ua_alloc(UArena *ua, size_t size)
{
	void *ptr = NULL;
	size_t aligned_sz = size + LmPow2AlignUp(size, ua->alignment);
	if (LM_LIKELY(ua->cur + aligned_sz <= ua->cap)) {
		ptr = ua->mem + ua->cur;
		ua->cur += aligned_sz;
	}

	return ptr;
}

void *ua_alloc_timed(UArena *ua, size_t sz)
{
	START_TSC_TIMING_LFENCE(alloc);
	//--------------------------------------
	void *ptr = ua_alloc(ua, sz);
	//--------------------------------------
	END_TSC_TIMING_LFENCE(alloc);
	uint64_t alloc_time = alloc_end - alloc_start;
#if 0
        static uint64_t print_when = 0;
	if ((print_when++ % 100) == 0)
		printf("Timing: %lu\n", alloc_time);
#endif
	timing_stats.ua_alloc_total_time += alloc_time;
	timing_stats.ua_alloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}

inline void *ua_zalloc(UArena *ua, size_t size)
{
	void *ptr = NULL;
	size_t aligned_sz = size + LmPow2AlignUp(size, ua->alignment);
	if (LM_LIKELY(ua->cur + aligned_sz <= ua->cap)) {
		ptr = ua->mem + ua->cur;
		ua->cur += aligned_sz;
		explicit_bzero(ptr, aligned_sz);
	}
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
	timing_stats.ua_zalloc_total_time += alloc_time;
	timing_stats.ua_zalloc_total_iter += 1;
	add_timing(alloc_time);
	//--------------------------------------
	return ptr;
}
