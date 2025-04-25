#include <src/lm.h>
LM_LOG_REGISTER(tests);

#include <src/cJSON/cJSON.h>
#include <src/allocators/allocator_wrappers.h>
#include <src/sdhs/Sdhs.h>

#include "network_test.h"
#include "tight_loop_test.h"

#include <stddef.h>
#include <sys/wait.h>

static const alloc_fn_t ua_alloc_functions[] = {
	ua_alloc_wrapper_timed /*,
						 ua_zalloc_wrapper_timed,
						 ua_falloc_wrapper_timed,
						 ua_fzalloc_wrapper_timed*/
};
static const free_fn_t ua_free_functions[] = { ua_free_wrapper };
static const realloc_fn_t ua_realloc_functions[] = { ua_realloc_wrapper_timed };

static const alloc_fn_t malloc_and_fam[] = { malloc_wrapper_timed,
					     calloc_wrapper_timed };
static const free_fn_t free_functions[] = { free_wrapper_timed };
static const realloc_fn_t realloc_functions[] = { realloc_wrapper_timed };

static const char *ua_alloc_function_names[] = { "alloc", "zalloc", "falloc",
						 "fzalloc" };
static const char *malloc_and_fam_names[] = { "malloc", "calloc" };

static void tight_loop_test_all_sizes(struct ua_params *params,
				      bool running_in_debugger,
				      uint64_t iterations, alloc_fn_t alloc_fn,
				      const char *alloc_fn_name,
				      const char *log_filename,
				      const char *file_mode)
{
#if 0
	if (iterations > 1000) {
		LmLogWarning(
			"%u iterations will allocate a lot of memory. Setting 'iterations'"
			" to 1000 instead (override must be disabled in the code)",
			iterations);
		iterations = 1000;
	}
#endif
	tight_loop_test(params, running_in_debugger, iterations, alloc_fn,
			alloc_fn_name, small_sizes, LmArrayLen(small_sizes),
			"small", log_filename, file_mode);

	tight_loop_test(params, running_in_debugger, iterations, alloc_fn,
			alloc_fn_name, medium_sizes, LmArrayLen(medium_sizes),
			"medium", log_filename, file_mode);

	tight_loop_test(params, running_in_debugger, iterations, alloc_fn,
			alloc_fn_name, large_sizes, LmArrayLen(large_sizes),
			"large", log_filename, file_mode);
}

static int u_arena_test(void *ctx, bool running_in_debugger)
{
	cJSON *ctx_json = ctx;
	cJSON *arena_sz_json = cJSON_GetObjectItem(ctx_json, "arena_sz");
	cJSON *alignment_json = cJSON_GetObjectItem(ctx_json, "alignment");
	cJSON *mallocd_json = cJSON_GetObjectItem(ctx_json, "mallocd");
	cJSON *contiguous_json = cJSON_GetObjectItem(ctx_json, "contiguous");
	cJSON *alloc_iterations_json =
		cJSON_GetObjectItem(ctx_json, "alloc_iterations");
	cJSON *log_filename_json =
		cJSON_GetObjectItem(ctx_json, "log_filename");
	LmAssert(arena_sz_json && alignment_json && mallocd_json &&
			 contiguous_json && alloc_iterations_json &&
			 log_filename_json,
		 "u_arena_test's context JSON is malformed");

	struct ua_params params = { 0 };

	params.arena_sz =
		lm_mem_sz_from_string(cJSON_GetStringValue(arena_sz_json));
	params.alignment = (size_t)cJSON_GetNumberValue(alignment_json);
	params.mallocd = cJSON_IsTrue(mallocd_json);
	params.contiguous = cJSON_IsTrue(contiguous_json);
	uint64_t alloc_iterations =
		(uint64_t)cJSON_GetNumberValue(alloc_iterations_json);
	char *log_filename = cJSON_GetStringValue(log_filename_json);

	LmAssert(alloc_iterations > 0, "u_arena_test's alloc_iterations is 0");

	LmLogDebug("test_params: %zd, %zd, %s, %s", params.arena_sz,
		   params.alignment, LmBoolToString(params.mallocd),
		   LmBoolToString(params.contiguous));

	const char *file_mode = "a";
	for (int i = 0; i < (int)LmArrayLen(ua_alloc_functions); ++i) {
		alloc_fn_t alloc_fn = ua_alloc_functions[i];
		const char *alloc_fn_name = ua_alloc_function_names[i];
		free_fn_t free_fn = ua_free_functions[0];
		realloc_fn_t realloc_fn = ua_realloc_functions[0];

#if 1
		tight_loop_test_all_sizes(&params, running_in_debugger,
					  alloc_iterations, alloc_fn,
					  alloc_fn_name, log_filename,
					  file_mode);
#endif

#if 0
		network_test(&params, alloc_fn, alloc_fn_name, free_fn,
			     realloc_fn, alloc_iterations, running_in_debugger,
			     log_filename, file_mode);
#endif
	}

	return 0;
}

static int malloc_test(void *ctx, bool running_in_debugger)
{
	cJSON *ctx_json = ctx;
	cJSON *alloc_iterations_json =
		cJSON_GetObjectItem(ctx_json, "alloc_iterations");
	cJSON *log_filename_json =
		cJSON_GetObjectItem(ctx_json, "log_filename");
	LmAssert(alloc_iterations_json && log_filename_json,
		 "malloc_test's context JSON is malformed");

	uint64_t alloc_iterations =
		(uint64_t)cJSON_GetNumberValue(alloc_iterations_json);
	char *log_filename = cJSON_GetStringValue(log_filename_json);

	LmAssert(alloc_iterations > 0, "malloc_test's alloc_iterations is 0");

	const char *file_mode = "a";
	for (int i = 0; i < (int)LmArrayLen(malloc_and_fam); ++i) {
		alloc_fn_t alloc_fn = malloc_and_fam[i];
		const char *alloc_fn_name = malloc_and_fam_names[i];
		free_fn_t free_fn = free_functions[0];
		realloc_fn_t realloc_fn = realloc_functions[0];

#if 1
		tight_loop_test_all_sizes(NULL, running_in_debugger,
					  alloc_iterations, alloc_fn,
					  alloc_fn_name, log_filename,
					  file_mode);
#endif

#if 1
		network_test(NULL, alloc_fn, alloc_fn_name, free_fn, realloc_fn,
			     alloc_iterations, running_in_debugger,
			     log_filename, file_mode);
#endif
	}
	return 0;
}

static int sdhs_test(void *ctx, bool running_in_debugger)
{
	(void)ctx;
	(void)running_in_debugger;
	SdhsMain(0, NULL);
	return 0;
}

static struct test_definition test_definitions[] = {
	{ u_arena_test, "u_arena_m_c" },
	{ u_arena_test, "u_arena_m_nc" },
	{ u_arena_test, "u_arena_nm_c" },
	{ u_arena_test, "u_arena_nm_nc" },
	{ malloc_test, "malloc" },
	{ sdhs_test, "sdhs" },
	{ 0 }
};

static struct test_definition *get_test_definition(cJSON *test_name_json)
{
	char *test_name = cJSON_GetStringValue(test_name_json);
	struct test_definition test_def = test_definitions[0];

	for (int i = 0; test_def.test != NULL;
	     test_def = test_definitions[++i]) {
		// TODO: (isa): Is strcmp fine here?
		if (strcmp(test_def.test_name, test_name) == 0)
			return &test_definitions[i];
	}

	LmLogWarning(
		"No definition found for test %s! Is it spelled incorecctly, either in the config or in the code?",
		test_name);
	return NULL;
}

int run_tests(cJSON *conf)
{
	if (!conf) {
		LmLogError("Suite JSON is NULL");
		return -1; // -1 is JSON error
	}

	cJSON *suite_enabled_json = cJSON_GetObjectItem(conf, "enabled");
	cJSON *running_in_debugger_json = cJSON_GetObjectItem(conf, "debugger");
	cJSON *tests_json = cJSON_GetObjectItem(conf, "tests");
	if (!(suite_enabled_json && running_in_debugger_json)) {
		LmLogError("No 'enabled' or 'debugger' field in suite JSON");
		return -1;
	}

	if (cJSON_IsFalse(suite_enabled_json)) {
		LmLogInfo("Tests are disabled");
		return 1;
	}

	cJSON *test_json;
	cJSON_ArrayForEach(test_json, tests_json)
	{
		cJSON *test_enabled_json =
			cJSON_GetObjectItem(test_json, "enabled");
		if (cJSON_IsTrue(test_enabled_json)) {
			cJSON *name_json =
				cJSON_GetObjectItem(test_json, "name");
			if (!name_json) {
				LmLogError("No 'name' field in test JSON");
				return -1;
			}

			cJSON *ctx_json = NULL;
			bool has_ctx = cJSON_HasObjectItem(test_json, "ctx");
			if (has_ctx)
				ctx_json =
					cJSON_GetObjectItem(test_json, "ctx");

			struct test_definition *test_definition =
				get_test_definition(name_json);

			int result = test_definition->test(
				ctx_json,
				cJSON_IsTrue(running_in_debugger_json));
			if (result != 0) {
				LmLogWarning("Test %s failed",
					     cJSON_GetStringValue(name_json));
				return result;
			}
		}
	}

	return 0;
}
