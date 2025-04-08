#include <src/lm.h>
LM_LOG_REGISTER(munit_tests);

#include "allocator_tests.h"
#include "munit_tests.h"

#include <src/allocators/u_arena.h>

#include <src/munit/munit.h>
#include <src/cJSON/cJSON.h>

#define ARENA_SZ LmKibiByte(512)

MunitResult u_arena_mallocd_cont_test(const MunitParameter params[], void *data)
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

MunitResult u_arena_mallocd_non_cont_test(const MunitParameter params[],
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

MunitResult u_arena_not_mallocd_cont_test(const MunitParameter params[],
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

MunitResult u_arena_not_mallocd_non_cont_test(const MunitParameter params[],
					      void *data)
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

MunitResult malloc_test(const MunitParameter params[], void *data)
{
	(void)params;
	(void)data;
	if (malloc_tests() == 0)
		return MUNIT_OK;
	else
		return MUNIT_FAIL;
}

struct setup_fn_name_map {
	munit_setup_fn setup_fn;
	const char *setup_fn_name;
};

struct teardown_fn_name_map {
	munit_teardown_fn teardown_fn;
	const char *teardown_fn_name;
};

struct test_fn_name_map {
	munit_test_fn test_fn;
	const char *test_fn_name;
};

// TODO: (isa): Make arrays for setup and teardown functions if applicable
static struct test_fn_name_map test_name_maps[] = {
	{ u_arena_mallocd_cont_test, LM_STRINGIFY(u_arena_mallocd_cont_test) },
	{ u_arena_mallocd_non_cont_test,
	  LM_STRINGIFY(u_arena_mallocd_non_cont_test) },
	{ u_arena_not_mallocd_cont_test,
	  LM_STRINGIFY(u_arena_not_mallocd_cont_test) },
	{ u_arena_not_mallocd_non_cont_test,
	  LM_STRINGIFY(u_arena_not_mallocd_non_cont_test) },
	{ 0 }
};

MunitSuite *get_sub_suites(cJSON *sub_suites_json)
{
	// TODO: (isa): Just a stub for now
	(void)sub_suites_json;
	return NULL;
}

MunitSuiteOptions get_suite_options(cJSON *suite_options_json)
{
	// TODO: (isa): Just a stub for now
	(void)suite_options_json;
	return MUNIT_SUITE_OPTION_NONE;
}

munit_setup_fn get_test_setup_fn(cJSON *setup_json)
{
	// TODO: (isa): Just a stub for now
	(void)setup_json;
	return NULL;
}

munit_teardown_fn get_test_teardown_fn(cJSON *teardown_json)
{
	// TODO: (isa): Just a stub for now
	(void)teardown_json;
	return NULL;
}

MunitTestOptions get_test_options(cJSON *test_options_json)
{
	// TODO: (isa): Just a stub for now
	(void)test_options_json;
	return MUNIT_TEST_OPTION_NONE;
}

munit_test_fn get_test_fn(cJSON *test_fn_json)
{
	char *test_fn_name = cJSON_GetStringValue(test_fn_json);
	struct test_fn_name_map test_name_map = test_name_maps[0];
	for (int i = 1; test_name_map.test_fn != NULL; ++i) {
		if (strcmp(test_name_map.test_fn_name, test_fn_name) == 0)
			return test_name_map.test_fn;
		else
			test_name_map = test_name_maps[i];
	}

	return NULL;
}

MunitTest *get_suite_tests(cJSON *suite_tests_json)
{
	MunitTest *tests = NULL;

	int enabled_test_count = 0;
	cJSON *test_json;
	cJSON_ArrayForEach(test_json, suite_tests_json)
	{
		cJSON *test_enabled_json =
			cJSON_GetObjectItem(test_json, "enabled");
		if (!test_enabled_json) {
			LmLogError("Malformed suite test JSON");
			return NULL;
		}
		if (cJSON_IsTrue(test_enabled_json))
			++enabled_test_count;
	}

	if (enabled_test_count == 0) {
		LmLogWarning("No tests in the suite were enabled");
		return NULL;
	}

	// NOTE: (isa): Using calloc ensures that the final entry will be a nil-entry
	tests = calloc(enabled_test_count + 1, sizeof(MunitTest));

	int test_json_idx = 0;
	cJSON_ArrayForEach(test_json, suite_tests_json)
	{
		cJSON *test_enabled_json =
			cJSON_GetObjectItem(test_json, "enabled");
		if (cJSON_IsTrue(test_enabled_json)) {
			cJSON *test_name_json =
				cJSON_GetObjectItem(test_json, "name");
			cJSON *test_fn_json =
				cJSON_GetObjectItem(test_json, "test_fn");
			cJSON *setup_fn_json =
				cJSON_GetObjectItem(test_json, "setup_fn");
			cJSON *teardown_fn_json =
				cJSON_GetObjectItem(test_json, "teardown_fn");
			cJSON *options_json =
				cJSON_GetObjectItem(test_json, "options");
			if (!test_name_json || !test_fn_json ||
			    !setup_fn_json || !teardown_fn_json ||
			    !options_json) {
				LmLogError("Malformed suite test JSON");
				free(tests);
				return NULL;
			}

			MunitTest *test = &tests[test_json_idx++];
			test->name = lm_string_make(
				cJSON_GetStringValue(test_name_json));
			test->test = get_test_fn(test_fn_json);
			test->setup = get_test_setup_fn(setup_fn_json);
			// NOTE: (isa): ^^^ Always returns NULL atm
			test->tear_down =
				get_test_teardown_fn(teardown_fn_json);
			// NOTE: (isa): ^^^ Always returns NULL atm
			test->options = get_test_options(options_json);
			// NOTE: (isa): ^^^ Always returns MUNIT_TEST_OPTION_NONE atm
			// TODO: (isa): Determine if test->parameters should be set here
		}
	}

	return tests;
}

MunitSuite *create_munit_suite(cJSON *suite_conf_json)
{
	MunitSuite *suite;

	cJSON *suite_enabled_json =
		cJSON_GetObjectItem(suite_conf_json, "enabled");
	if (!suite_enabled_json) {
		LmLogError("Malformed suite configuration JSON");
		return NULL;
	}
	if (cJSON_IsFalse(suite_enabled_json))
		return NULL;

	cJSON *suite_prefix_json =
		cJSON_GetObjectItem(suite_conf_json, "prefix");
	cJSON *sub_suites_json =
		cJSON_GetObjectItem(suite_conf_json, "sub_suites");
	cJSON *iterations_json =
		cJSON_GetObjectItem(suite_conf_json, "iterations");
	cJSON *suite_options_json =
		cJSON_GetObjectItem(suite_conf_json, "options");
	cJSON *suite_tests_json = cJSON_GetObjectItem(suite_conf_json, "tests");
	if (!suite_prefix_json || !sub_suites_json || !iterations_json ||
	    !suite_options_json || !suite_tests_json) {
		LmLogError("Malformed suite configuration JSON");
		return NULL;
	}

	suite = malloc(sizeof(MunitSuite));
	suite->prefix = lm_string_make(cJSON_GetStringValue(suite_prefix_json));
	suite->suites = get_sub_suites(sub_suites_json);
	// NOTE: (isa): ^^^ Always returns NULL atm
	suite->iterations = cJSON_GetNumberValue(iterations_json);
	suite->options = get_suite_options(suite_options_json);
	// NOTE: (isa): ^^^ Always returns MUNIT_SUITE_OPTION_NONE atm
	suite->tests = get_suite_tests(suite_tests_json);

	return suite;
}
