#include <src/lm.h>
LM_LOG_REGISTER(tests);

#include <src/cJSON/cJSON.h>
#include <src/allocators/allocator_wrappers.h>
#include <src/sdhs/Sdhs.h>

#include "network_test.h"
#include "tight_loop_test.h"

#include <stddef.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>

extern UArena *main_ua;

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

// NOTE: (isa): Made by Claude
static int get_next_dir_num(LmString directory)
{
	DIR *dir;
	struct dirent *entry;
	int largest_num = 0;
	int current_num;

	dir = opendir(directory);
	if (dir == NULL) {
		LmLogError("Unable to open directory: %s", strerror(errno));
		return -errno;
	}

	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		if (entry->d_type == DT_DIR) {
			current_num = atoi(entry->d_name);

			if (current_num > 0 ||
			    (current_num == 0 &&
			     strcmp(entry->d_name, "0") == 0)) {
				if (current_num > largest_num) {
					largest_num = current_num;
				}
			}
		}
	}

	closedir(dir);
	return largest_num + 1;
}

static void
tight_loop_test_all_sizes(struct ua_params *params, bool running_in_debugger,
			  uint64_t iterations, alloc_fn_t alloc_fn,
			  const char *alloc_fn_name, LmString log_filename,
			  const char *file_mode, const char *log_filename_base)
{
	tight_loop_test(params, running_in_debugger, iterations, alloc_fn,
			alloc_fn_name, small_sizes, LmArrayLen(small_sizes),
			"small", log_filename, file_mode, log_filename_base);

	tight_loop_test(params, running_in_debugger, iterations, alloc_fn,
			alloc_fn_name, medium_sizes, LmArrayLen(medium_sizes),
			"medium", log_filename, file_mode, log_filename_base);

	tight_loop_test(params, running_in_debugger, iterations, alloc_fn,
			alloc_fn_name, large_sizes, LmArrayLen(large_sizes),
			"large", log_filename, file_mode, log_filename_base);
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
	cJSON *log_directory_json =
		cJSON_GetObjectItem(ctx_json, "log_directory");
	LmAssert(arena_sz_json && alignment_json && mallocd_json &&
			 contiguous_json && alloc_iterations_json &&
			 log_directory_json,
		 "u_arena_test's context JSON is malformed");

	struct ua_params params = { 0 };

	params.arena_sz =
		lm_mem_sz_from_string(cJSON_GetStringValue(arena_sz_json));
	params.alignment = (size_t)cJSON_GetNumberValue(alignment_json);
	params.mallocd = cJSON_IsTrue(mallocd_json);
	params.contiguous = cJSON_IsTrue(contiguous_json);
	uint64_t alloc_iterations =
		(uint64_t)cJSON_GetNumberValue(alloc_iterations_json);

	LmString log_directory = lm_string_make(
		cJSON_GetStringValue(log_directory_json), main_ua);
	int ret = mkdir(log_directory, S_IRWXU);
	if (ret != 0) {
		if (errno != EEXIST) {
			LmLogError("Unable to create directory %s: %s",
				   log_directory, strerror(errno));
			return -errno;
		}

		int next_dirn = get_next_dir_num(log_directory);
		if (next_dirn < 0)
			return next_dirn;

		lm_string_append_fmt(log_directory, "%d/", next_dirn);

		ret = mkdir(log_directory, S_IRWXU);
		if (ret != 0) {
			LmLogError("Unable to create directory %s: %s",
				   log_directory, strerror(errno));
			return -errno;
		}
	}

	LmString log_filename = lm_string_make(log_directory, main_ua);
	lm_string_append_fmt(log_filename, "%s", "log.txt");

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
					  file_mode, log_directory);
#endif

#if 0
		network_test(&params, alloc_fn, alloc_fn_name, free_fn,
			     realloc_fn, alloc_iterations, running_in_debugger,
			     log_filename, file_mode);
#endif
	}

	return 0;
}

// static void karena_tight_loop(uint64_t alloc_iterations, char *log_filename,
// 			      const array_test *test)
// {
// 	FILE *log_file = lm_open_file_by_name(log_filename, "a");
// 	LmSetLogFileLocal(log_file);
//
// 	size_t arena_size = test->array[test->len] * alloc_iterations;
//
// 	LmLogDebugR("\n------------------------------");
// 	LmLogDebug("%s -- %s", "karena", test->name);
//
// 	for (size_t j = 0; j < test->len; ++j) {
// 		void *a = karena_create(arena_size);
// 		LmLogDebugR("\n%s'ing %zd bytes %lu times", "alloc",
// 			    test->array[j], alloc_iterations);
//
// 		TIME_TIGHT_LOOP(PROC_CPUTIME, alloc_iterations,
// 				uint8_t *ptr = karena_alloc(a, test->array[j]);
// 				*ptr = 1;);
// 	}
//
// 	LmRemoveLogFileLocal();
// 	lm_close_file(log_file);
// }
//
// int karena_tests(void *ctx, bool running_in_debugger)
// {
// 	(void)running_in_debugger;
// 	cJSON *ctx_json = ctx;
// 	cJSON *alloc_iterations_json =
// 		cJSON_GetObjectItem(ctx_json, "alloc_iterations");
// 	cJSON *log_filename_json =
// 		cJSON_GetObjectItem(ctx_json, "log_filename");
//
// 	uint64_t alloc_iterations =
// 		(uint64_t)cJSON_GetNumberValue(alloc_iterations_json);
// 	char *log_filename = cJSON_GetStringValue(log_filename_json);
//
// 	array_test small = {
// 		.array = small_sizes,
// 		.len = LmArrayLen(small_sizes),
// 		.name = "small",
// 	};
//
// 	array_test medium = {
// 		.array = medium_sizes,
// 		.len = LmArrayLen(medium_sizes),
// 		.name = "medium",
// 	};
//
// 	array_test large = { .array = large_sizes,
// 			     .len = LmArrayLen(large_sizes),
// 			     .name = "large" };
//
// 	karena_tight_loop(alloc_iterations, log_filename, &small);
// 	karena_tight_loop(alloc_iterations, log_filename, &medium);
// 	// large no work ):
// 	// karena_tight_loop(params, &large);
// 	return 0;
// }

static int malloc_test(void *ctx, bool running_in_debugger)
{
	cJSON *ctx_json = ctx;
	cJSON *alloc_iterations_json =
		cJSON_GetObjectItem(ctx_json, "alloc_iterations");
	cJSON *log_directory_json =
		cJSON_GetObjectItem(ctx_json, "log_directory");
	LmAssert(alloc_iterations_json && log_directory_json,
		 "malloc_test's context JSON is malformed");

	uint64_t alloc_iterations =
		(uint64_t)cJSON_GetNumberValue(alloc_iterations_json);
	char *log_directory = cJSON_GetStringValue(log_directory_json);
	int ret = mkdir(log_directory, S_IRWXU);
	if (ret != 0 && errno != EEXIST) {
		LmLogError("Failed to create directory %s: %s", log_directory,
			   strerror(errno));
		return -errno;
	}

	LmString log_filename = lm_string_make(log_directory, main_ua);
	lm_string_append_fmt(log_filename, "%s", "log.txt");

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
					  file_mode, log_directory);
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
