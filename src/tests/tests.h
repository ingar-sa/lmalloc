#ifndef TESTS_H
#define TESTS_H

#include <src/lm.h>
#include <src/allocators/u_arena.h>
#include <src/metrics/timing.h>
#include <src/allocators/allocator_wrappers.h>
#include <src/cJSON/cJSON.h>

// NOTE: (isa): Start arrays created by Claude
static size_t small_sizes[] = { 8,  13, 16, 27, 32,  45,  56,  64,
				71, 80, 91, 96, 103, 112, 125, 128 };

static size_t medium_sizes[] = {
	256,  300,  378,  512,	768,  818,  1000, 1024,
	1359, 1700, 2000, 2048, 2222, 2918, 3875, 4096
};

static size_t large_sizes[] = {
	LmKibiByte(5) + 35, /* ~16 KB (unaligned) */
	LmKibiByte(7), /* ~16 KB (unaligned) */
#if 0
	LmKibiByte(32), /* 32 KB (aligned) */
	LmKibiByte(64) + 127, /* ~64 KB (unaligned) */
	LmKibiByte(128), /* 128 KB (aligned) */
	LmKibiByte(256) + 511, /* ~256 KB (unaligned) */
	LmKibiByte(512), /* 512 KB (aligned) */
	LmKibiByte(1024) + 973, /* ~1 MB (unaligned) */
	LmKibiByte(1536), /* 1.5 MB (aligned) */
	LmKibiByte(2048) + 1523, /* ~2 MB (unaligned) */
	LmKibiByte(2560), /* 2.5 MB (aligned) */
	LmKibiByte(3072) + 2731, /* ~3 MB (unaligned) */
	LmKibiByte(4096), /* 4 MB (aligned) */
	LmKibiByte(5120) + 3567, /* ~5 MB (unaligned) */
	LmKibiByte(6144), /* 6 MB (aligned) */
	LmKibiByte(7168) + 4095, /* ~7 MB (unaligned) */
	LmKibiByte(8192) /* 8 MB (aligned) */
#endif
};

// NOTE: (isa): End arrays created by Claude

typedef struct {
	size_t *array;
	size_t len;
	const char *name;
} array_test;

typedef int (*test_fn_t)(void *ctx, bool running_in_debugger);

struct test_definition {
	test_fn_t test;
	const char *test_name;
};

int run_tests(cJSON *conf);

struct ua_params {
	size_t arena_sz;
	size_t alignment;
	bool contiguous;
	bool mallocd;
};

#endif
