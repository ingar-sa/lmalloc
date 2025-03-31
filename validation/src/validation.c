#define LM_H_IMPLEMENTATION
#include <src/lm.h>
#undef LM_H_IMPLEMENTATION

LM_LOG_GLOBAL_REGISTER();
LM_LOG_REGISTER(validation);

#include <src/metrics/timing.h>
#include <src/u_arena/u_arena.h>

#include <src/munit/munit.h>

#include <stdlib.h>
#include <stddef.h>

struct arena_test_params {
	size_t cap;
	size_t alignment;
	int n_small_allocs;
	int n_large_allocs;
};

/*
 * Time creation
 * Test allocation alignment
 * Time free
 * Time small allocations (average)
 * Time large allocations (average)
*/
static MunitResult u_arena_test(const MunitParameter params[], void *data)
{
	(void)params;
	struct arena_test_params *test_params = data;

	LM_START_TIMING(create, PROC_CPUTIME);
	u_arena *a = u_arena_create(test_params->cap, U_ARENA_NON_CONTIGUOUS,
				    test_params->alignment);
	LM_END_TIMING(create, PROC_CPUTIME);
	LM_LOG_TIMING(create, "Arena creation", US, DBG, LM_LOG_MODULE_LOCAL);

	ptrdiff_t a_to_a_mem_diff = LmPtrDiff(a->mem, a);
	LmLogDebug("Distance from arena to its memory%zd", a_to_a_mem_diff);

	/* Allocation size less than alignment */
	size_t alloc_sz = test_params->alignment - 1;

	LM_START_TIMING(small_alloc_lt_aligned_sz, PROC_CPUTIME);
	for (int i = 0; i < test_params->n_small_allocs; ++i) {
		uint8_t *allocation = u_arena_alloc(a, alloc_sz);
	}
	u_arena_free(a);
	LM_END_TIMING(small_alloc_lt_aligned_sz, PROC_CPUTIME);

	LM_LOG_TIMING_AVG(small_alloc_lt_aligned_sz,
			  test_params->n_small_allocs,
			  "Small allocation < aligned size average", US, DBG,
			  LM_LOG_MODULE_LOCAL);

	/* Allocation size same as alignment */
	alloc_sz = a->alignment;

	LM_START_TIMING(small_alloc_aligned_sz, PROC_CPUTIME);
	for (int i = 0; i < test_params->n_small_allocs; ++i) {
		uint8_t *allocation = u_arena_alloc(a, alloc_sz);
	}
	u_arena_free(a);
	LM_END_TIMING(small_alloc_aligned_sz, PROC_CPUTIME);

	LM_LOG_TIMING_AVG(small_alloc_aligned_sz, test_params->n_small_allocs,
			  "Small allocation aligned size average", US, DBG,
			  LM_LOG_MODULE_LOCAL);

	/* Allocation size greater than alignment */
	alloc_sz = a->alignment + 1;

	LM_START_TIMING(small_alloc_gt_aligned_sz, PROC_CPUTIME);
	for (int i = 0; i < test_params->n_small_allocs; ++i) {
		uint8_t *allocation = u_arena_alloc(a, alloc_sz);
	}
	u_arena_free(a);
	LM_END_TIMING(small_alloc_gt_aligned_sz, PROC_CPUTIME);

	LM_LOG_TIMING_AVG(small_alloc_gt_aligned_sz,
			  test_params->n_small_allocs,
			  "Small allocation > aligned size average", US, DBG,
			  LM_LOG_MODULE_LOCAL);

	return MUNIT_OK;
}

static MunitResult u_arena_contiguous_test(const MunitParameter params[],
					   void *data)
{
	(void)params;
	struct arena_test_params *test_params = data;

	LM_START_TIMING(create, PROC_CPUTIME);
	u_arena *a = u_arena_create(test_params->cap, U_ARENA_CONTIGUOUS,
				    test_params->alignment);
	LM_END_TIMING(create, PROC_CPUTIME);
	LM_LOG_TIMING(create, "Arena creation", US, DBG, LM_LOG_MODULE_LOCAL);

	ptrdiff_t a_to_a_mem_diff = LmPtrDiff(a->mem, a);
	LmLogDebug("Distance from arena to its memory%zd", a_to_a_mem_diff);

	/* Allocation size less than alignment */
	size_t alloc_sz = test_params->alignment - 1;

	LM_START_TIMING(small_alloc_lt_aligned_sz, PROC_CPUTIME);
	for (int i = 0; i < test_params->n_small_allocs; ++i) {
		uint8_t *allocation = u_arena_alloc(a, alloc_sz);
	}
	u_arena_free(a);
	LM_END_TIMING(small_alloc_lt_aligned_sz, PROC_CPUTIME);

	LM_LOG_TIMING_AVG(small_alloc_lt_aligned_sz,
			  test_params->n_small_allocs,
			  "Small allocation < aligned size average", US, DBG,
			  LM_LOG_MODULE_LOCAL);

	/* Allocation size same as alignment */
	alloc_sz = a->alignment;

	LM_START_TIMING(small_alloc_aligned_sz, PROC_CPUTIME);
	for (int i = 0; i < test_params->n_small_allocs; ++i) {
		uint8_t *allocation = u_arena_alloc(a, alloc_sz);
	}
	u_arena_free(a);
	LM_END_TIMING(small_alloc_aligned_sz, PROC_CPUTIME);

	LM_LOG_TIMING_AVG(small_alloc_aligned_sz, test_params->n_small_allocs,
			  "Small allocation aligned size average", US, DBG,
			  LM_LOG_MODULE_LOCAL);

	/* Allocation size greater than alignment */
	alloc_sz = a->alignment + 1;

	LM_START_TIMING(small_alloc_gt_aligned_sz, PROC_CPUTIME);
	for (int i = 0; i < test_params->n_small_allocs; ++i) {
		uint8_t *allocation = u_arena_alloc(a, alloc_sz);
	}
	u_arena_free(a);
	LM_END_TIMING(small_alloc_gt_aligned_sz, PROC_CPUTIME);

	LM_LOG_TIMING_AVG(small_alloc_gt_aligned_sz,
			  test_params->n_small_allocs,
			  "Small allocation > aligned size average", US, DBG,
			  LM_LOG_MODULE_LOCAL);

	return MUNIT_OK;
}

static MunitResult malloc_test(const MunitParameter params[], void *data)
{
	(void)params;
	struct arena_test_params *test_params = data;
	uint8_t *allocation;

	/* Allocation size less than (arena) alignment */
	size_t alloc_sz = test_params->alignment - 1;
	LM_START_TIMING(small_alloc_lt_aligned_sz, PROC_CPUTIME);
	for (int i = 0; i < test_params->n_small_allocs; ++i) {
		allocation = malloc(alloc_sz);
	}
	LM_END_TIMING(small_alloc_lt_aligned_sz, PROC_CPUTIME);

	LM_LOG_TIMING_AVG(small_alloc_lt_aligned_sz,
			  test_params->n_small_allocs,
			  "Small allocation < aligned size average", US, DBG,
			  LM_LOG_MODULE_LOCAL);

	/* Allocation size same as (arena) alignment */
	alloc_sz = test_params->alignment;
	LM_START_TIMING(small_alloc_aligned_sz, PROC_CPUTIME);
	for (int i = 0; i < test_params->n_small_allocs; ++i) {
		allocation = malloc(alloc_sz);
	}
	LM_END_TIMING(small_alloc_aligned_sz, PROC_CPUTIME);

	LM_LOG_TIMING_AVG(small_alloc_aligned_sz, test_params->n_small_allocs,
			  "Small allocation aligned size average", US, DBG,
			  LM_LOG_MODULE_LOCAL);

	/* Allocation size greater than (arena) alignment */
	alloc_sz = test_params->alignment + 1;
	LM_START_TIMING(small_alloc_gt_aligned_sz, PROC_CPUTIME);
	for (int i = 0; i < test_params->n_small_allocs; ++i) {
		allocation = malloc(alloc_sz);
	}
	LM_END_TIMING(small_alloc_gt_aligned_sz, PROC_CPUTIME);

	LM_LOG_TIMING_AVG(small_alloc_gt_aligned_sz,
			  test_params->n_small_allocs,
			  "Small allocation > aligned size average", US, DBG,
			  LM_LOG_MODULE_LOCAL);

	return MUNIT_OK;
}
static MunitTest tests[] = {
	{ (char *)"u_arena non-contiguous", u_arena_test, NULL, NULL,
	  MUNIT_TEST_OPTION_NONE, NULL },
	{ (char *)"u_arena contiguous", u_arena_contiguous_test, NULL, NULL,
	  MUNIT_TEST_OPTION_NONE, NULL },
	{ (char *)"malloc", malloc_test, NULL, NULL, MUNIT_TEST_OPTION_NONE,
	  NULL },
	{ "", NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL }
};

static const MunitSuite test_suite = { (char *)"", tests, NULL, 1,
				       MUNIT_SUITE_OPTION_NONE };

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
	//const int n = 20;
	//long long sum = 0;
	//for (int i = 0; i < n; ++i) {
	//	long long foo = lm_get_time_stamp(PROC_CPUTIME);
	//	long long bar = lm_get_time_stamp(PROC_CPUTIME);
	//	//sum += bar - foo;
	//	lm_log_timing(bar - foo, "Timing time", NS, DBG,
	//		      LM_LOG_MODULE_LOCAL);
	//}
	//lm_log_timing_avg(sum, n, "Timing overhead", NS, DBG,
	//		  LM_LOG_MODULE_LOCAL);

	struct arena_test_params test_params = { LmKibiByte(512), 16, 100000,
						 1000 };
	int success = munit_suite_main(&test_suite, &test_params, argc, argv);
	return EXIT_SUCCESS;
}
