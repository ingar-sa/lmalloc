#define LM_H_IMPLEMENTATION
#include <src/lm.h>
#undef LM_H_IMPLEMENTATION

LM_LOG_GLOBAL_REGISTER();
LM_LOG_REGISTER(validation);

#include <src/metrics/timing.h>
#include <src/tests/allocator_tests.h>
#include <src/allocators/u_arena.h>

#include <src/munit/munit.h>

#include <stdlib.h>
#include <stddef.h>

#define ARENA_SZ LmKibiByte(512)

static MunitResult u_arena_mallocd_cont_test(const MunitParameter params[],
					     void *data)
{
	(void)params;
	(void)data;
	struct u_arena_test_params test_params = { ARENA_SZ, 16,
						   U_ARENA_CONTIGUOUS,
						   U_ARENA_MALLOCD };
	if (u_arena_tests(test_params) == 0)
		return MUNIT_OK;
	else
		return MUNIT_FAIL;
}

static MunitResult u_arena_mallocd_non_cont_test(const MunitParameter params[],
						 void *data)
{
	(void)params;
	(void)data;
	struct u_arena_test_params test_params = { ARENA_SZ, 16,
						   U_ARENA_NON_CONTIGUOUS,
						   U_ARENA_MALLOCD };
	if (u_arena_tests(test_params) == 0)
		return MUNIT_OK;
	else
		return MUNIT_FAIL;
}
static MunitResult u_arena_not_mallocd_cont_test(const MunitParameter params[],
						 void *data)
{
	(void)params;
	(void)data;
	struct u_arena_test_params test_params = { ARENA_SZ, 16,
						   U_ARENA_CONTIGUOUS,
						   U_ARENA_NOT_MALLOCD };
	if (u_arena_tests(test_params) == 0)
		return MUNIT_OK;
	else
		return MUNIT_FAIL;
}

static MunitResult
u_arena_not_mallocd_non_cont_test(const MunitParameter params[], void *data)
{
	(void)params;
	(void)data;
	struct u_arena_test_params test_params = { ARENA_SZ, 16,
						   U_ARENA_NON_CONTIGUOUS,
						   U_ARENA_NOT_MALLOCD };
	if (u_arena_tests(test_params) == 0)
		return MUNIT_OK;
	else
		return MUNIT_FAIL;
}

static MunitResult malloc_test(const MunitParameter params[], void *data)
{
	(void)params;
	(void)data;
	if (malloc_tests() == 0)
		return MUNIT_OK;
	else
		return MUNIT_FAIL;
}

// NOTE: (isa): Defines created by Claude
#define U_ARENA_MALLOCD_CONT_TEST_ENABLED 1
#define U_ARENA_MALLOCD_NON_CONT_TEST_ENABLED 1
#define U_ARENA_NOT_MALLOCD_CONT_TEST_ENABLED 1
#define U_ARENA_NOT_MALLOCD_NON_CONT_TEST_ENABLED 1
#define MALLOC_TEST_ENABLED 1

/* Test definitions */
#if U_ARENA_MALLOCD_CONT_TEST_ENABLED
#define U_ARENA_MALLOCD_CONT_TEST               \
	{ (char *)"u_arena mallocd contiguous", \
	  u_arena_mallocd_cont_test,            \
	  NULL,                                 \
	  NULL,                                 \
	  MUNIT_TEST_OPTION_NONE,               \
	  NULL },
#else
#define U_ARENA_MALLOCD_CONT_TEST
#endif

#if U_ARENA_NOT_MALLOCD_NON_CONT_TEST_ENABLED
#define U_ARENA_MALLOCD_NON_CONT_TEST               \
	{ (char *)"u_arena mallocd non-contiguous", \
	  u_arena_mallocd_non_cont_test,            \
	  NULL,                                     \
	  NULL,                                     \
	  MUNIT_TEST_OPTION_NONE,                   \
	  NULL },
#else
#define U_ARENA_MALLOCD_NON_CONT_TEST
#endif

#if U_ARENA_NOT_MALLOCD_CONT_TEST_ENABLED
#define U_ARENA_NOT_MALLOCD_CONT_TEST               \
	{ (char *)"u_arena not mallocd contiguous", \
	  u_arena_not_mallocd_cont_test,            \
	  NULL,                                     \
	  NULL,                                     \
	  MUNIT_TEST_OPTION_NONE,                   \
	  NULL },
#else
#define U_ARENA_NOT_MALLOCD_CONT_TEST
#endif

#if U_ARENA_NOT_MALLOCD_NON_CONT_TEST_ENABLED
#define U_ARENA_NOT_MALLOCD_NON_CONT_TEST                \
	{ (char *)"um_arena not mallocd non-contiguous", \
	  u_arena_not_mallocd_non_cont_test,             \
	  NULL,                                          \
	  NULL,                                          \
	  MUNIT_TEST_OPTION_NONE,                        \
	  NULL },
#else
#define U_ARENA_NOT_MALLOCD_NON_CONT_TEST
#endif

#if MALLOC_TEST_ENABLED
#define MALLOC_TEST                                        \
	{ (char *)"malloc",	  malloc_test, NULL, NULL, \
	  MUNIT_TEST_OPTION_NONE, NULL },
#else
#define MALLOC_TEST
#endif

#define FINAL_NULL_ENTRY { 0 }

static MunitTest tests[] = {
	U_ARENA_MALLOCD_CONT_TEST U_ARENA_MALLOCD_NON_CONT_TEST
		U_ARENA_NOT_MALLOCD_CONT_TEST U_ARENA_NOT_MALLOCD_NON_CONT_TEST
			MALLOC_TEST FINAL_NULL_ENTRY
};

static const MunitSuite test_suite = { (char *)"", tests, NULL, 1,
				       MUNIT_SUITE_OPTION_NONE };

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
	(void)argv;

	LmLogInfoGR("\n\n-----New run of allocator tests-----");
	int success = munit_suite_main(&test_suite, NULL, argc, argv);

	return EXIT_SUCCESS;
}
