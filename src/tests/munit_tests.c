#include <src/lm.h>
LM_LOG_REGISTER(munit_tests);

#include "allocator_tests.h"
#include "munit_tests.h"

#include <src/allocators/u_arena.h>

#include <src/munit/munit.h>
#include <src/cJSON/cJSON.h>

#define ARENA_SZ LmKibiByte(512)

#define BOOL_STRING(bool) (bool ? "true" : "false")

MunitResult u_arena_test(const MunitParameter mu_params[], void *data)
{
	(void)mu_params;
	struct u_arena_test_params *params = data;
	LmAssert(params->alloc_iterations > 0,
		 "Number of allocation iterations is 0");

	LmLogDebug("Params: %zd, %zd, %s, %s", params->arena_sz,
		   params->alignment, BOOL_STRING(params->mallocd),
		   BOOL_STRING(params->contiguous));

	MunitResult success = (u_arena_tests(params) == 0) ? MUNIT_OK :
							     MUNIT_FAIL;
	return success;
}

void *u_arena_test_setup(const MunitParameter *mu_params, void *data,
			 void *test_ctx)
{
	(void)mu_params;
	(void)data;

	cJSON *ctx_json = test_ctx;
	cJSON *arena_sz_json = cJSON_GetObjectItem(ctx_json, "arena_sz");
	cJSON *alignment_json = cJSON_GetObjectItem(ctx_json, "alignment");
	cJSON *mallocd_json = cJSON_GetObjectItem(ctx_json, "mallocd");
	cJSON *contiguous_json = cJSON_GetObjectItem(ctx_json, "contiguous");
	cJSON *alloc_iterations_json =
		cJSON_GetObjectItem(ctx_json, "alloc_iterations");
	LmAssert(arena_sz_json && alignment_json && mallocd_json &&
			 contiguous_json && alloc_iterations_json,
		 "u_arena_test's context JSON is malformed");

	struct u_arena_test_params *test_params =
		malloc(sizeof(struct u_arena_test_params));
	test_params->arena_sz =
		lm_mem_sz_from_string(cJSON_GetStringValue(arena_sz_json));
	test_params->alignment = cJSON_GetNumberValue(alignment_json);
	test_params->mallocd = cJSON_IsTrue(mallocd_json);
	test_params->contiguous = cJSON_IsTrue(contiguous_json);
	test_params->alloc_iterations =
		cJSON_GetNumberValue(alloc_iterations_json);
	LmAssert(test_params->alloc_iterations > 0,
		 "u_arena_test's alloc_iterations is 0");

	return test_params;
}

MunitResult malloc_test(const MunitParameter mu_params[], void *data)
{
	(void)mu_params;
	struct malloc_test_params *params = data;
	MunitResult result = (malloc_tests(params) == 0) ? MUNIT_OK :
							   MUNIT_FAIL;
	return result;
}

struct test_definition {
	MunitTestFunc test_fn;
	MunitTestSetup setup_fn;
	MunitTestTearDown teardown_fn;
	const char *test_name;
};
void *malloc_test_setup(const MunitParameter *mu_params, void *data,
			void *test_ctx)
{
	(void)mu_params;
	(void)data;

	cJSON *ctx_json = test_ctx;
	cJSON *alloc_iterations_json =
		cJSON_GetObjectItem(ctx_json, "alloc_iterations");
	LmAssert(alloc_iterations_json,
		 "malloc_test's context JSON is malformed");

	struct malloc_test_params *test_params =
		malloc(sizeof(struct malloc_test_params));
	test_params->alloc_iterations =
		cJSON_GetNumberValue(alloc_iterations_json);
	LmAssert(test_params->alloc_iterations > 0,
		 "malloc_test's alloc_iterations is 0");

	return test_params;
}

static struct test_definition test_definitions[] = {
	{ u_arena_test, u_arena_test_setup, NULL, "u_arena_mallocd_cont_test" },
	{ u_arena_test, u_arena_test_setup, NULL,
	  "u_arena_mallocd_non_cont_test" },
	{ u_arena_test, u_arena_test_setup, NULL,
	  "u_arena_not_mallocd_cont_test" },
	{ u_arena_test, u_arena_test_setup, NULL,
	  "u_arena_not_mallocd_non_cont_test" },
	{ malloc_test, NULL, NULL, "malloc_test" },
	{ 0 }
};

MunitSuite *get_sub_suites_STUB(cJSON *sub_suites_json)
{
	(void)sub_suites_json;
	return NULL;
}

MunitSuiteOptions get_suite_options_STUB(cJSON *suite_options_json)
{
	(void)suite_options_json;
	return MUNIT_SUITE_OPTION_NONE;
}

MunitTestOptions get_test_options_STUB(cJSON *test_options_json)
{
	(void)test_options_json;
	return MUNIT_TEST_OPTION_NONE;
}

struct test_definition *get_test_definition(cJSON *test_name_json)
{
	char *test_name = cJSON_GetStringValue(test_name_json);
	struct test_definition test_def = test_definitions[0];

	for (int i = 0; test_def.test_fn != NULL;
	     test_def = test_definitions[++i]) {
		// TODO: (isa): Is strcmp fine here?
		if (strcmp(test_def.test_name, test_name) == 0)
			return &test_definitions[i];
	}

	LmLogWarning(
		"No definition found with this name! Is it spelled incorecctly, either in the config or in the code?");
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
		LmAssert(test_enabled_json, "Malformed suite test JSON");

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
			cJSON *name_json =
				cJSON_GetObjectItem(test_json, "name");
			cJSON *options_json =
				cJSON_GetObjectItem(test_json, "options");
			LmAssert(name_json && options_json,
				 "Malformed suite test JSON");

			cJSON *ctx_json;
			bool has_ctx = cJSON_HasObjectItem(test_json, "ctx");
			if (has_ctx)
				ctx_json =
					cJSON_GetObjectItem(test_json, "ctx");
			MunitTest *test = &tests[test_json_idx++];
			struct test_definition *test_definition =
				get_test_definition(name_json);
			LmAssert(
				!(has_ctx &&
				  (test_definition->setup_fn == NULL)),
				"The test %s has a context in its JSON but no setup function",
				test_definition->test_name);
			test->name = lm_string_make(test_definition->test_name);
			test->test = test_definition->test_fn;
			test->setup = test_definition->setup_fn;
			test->tear_down = test_definition->teardown_fn;
			test->options = get_test_options_STUB(options_json);
			test->ctx = (has_ctx ? ctx_json : NULL);
		}
	}

	return tests;
}

MunitSuite *create_munit_suite(cJSON *suite_conf_json)
{
	cJSON *suite_enabled_json =
		cJSON_GetObjectItem(suite_conf_json, "enabled");
	LmAssert(suite_enabled_json, "Malformed suite configuration JSON");

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
	LmAssert(suite_prefix_json && sub_suites_json && iterations_json &&
			 suite_options_json && suite_tests_json,
		 "Malformed suite configuration JSON");

	MunitSuite *suite = malloc(sizeof(MunitSuite));
	suite->prefix = lm_string_make(cJSON_GetStringValue(suite_prefix_json));
	suite->suites = get_sub_suites_STUB(sub_suites_json);
	suite->iterations = cJSON_GetNumberValue(iterations_json);
	suite->options = get_suite_options_STUB(suite_options_json);
	suite->tests = get_suite_tests(suite_tests_json);

	return suite;
}
