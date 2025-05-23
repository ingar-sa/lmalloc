#include <src/lm.h>
LM_LOG_REGISTER(tests);

#include <src/cJSON/cJSON.h>
#include <src/allocators/allocator_wrappers.h>
#include <src/sdhs/Sdhs.h>
#include <src/utils/system_info.h>

// NOTE: (isa): Network test has been moved to "poc" for now,
// since it's not that interesting for an arena example
//#include "network_test.h"
#include "tight_loop_test.h"

#include <stddef.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <dirent.h>

extern UArena *main_ua;

static const alloc_fn_t a_alloc_functions[] = {
	oka_alloc_timed, ka_alloc_timed, ua_alloc_timed,
	// ua_zalloc_timed,
	//ua_falloc_timed
	//ua_fzalloc_timed
};

static const realloc_fn_t a_realloc_functions[] = { ua_realloc_timed };

static const alloc_fn_t malloc_and_fam[] = { malloc_timed };
static const realloc_fn_t realloc_functions[] = { realloc_timed };

static const char *a_alloc_function_names[] = {
	"okalloc", "kalloc", "ualloc", "zalloc", "falloc", "fzalloc"
};
static const char *malloc_and_fam_names[] = { "malloc" };

// NOTE: (isa): Claude
static int get_next_run_nr(LmString directory)
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
		if (entry->d_type == DT_DIR)
			continue;

		UAScratch uas = ua_scratch_begin(main_ua);
		LmString filename = lm_string_make(entry->d_name, uas.ua);
		lm_string_backspace(filename, 4); // "Remove" extension

		current_num = atoi(filename);
		if (current_num > 0) {
			if (current_num > largest_num) {
				largest_num = current_num;
			}
		}

		ua_scratch_release(uas);
	}

	closedir(dir);
	return (largest_num == 0) ? 1 : largest_num + 1;
}

static int make_dir(LmString log_directory)
{
	int ret = mkdir(log_directory, S_IRWXU);
	if (ret != 0 && errno != EEXIST) {
		LmLogError("Unable to create directory %s: %s", log_directory,
			   strerror(errno));
		return -errno;
	}

	return 0;
}

static void tight_loop_test_all_sizes(struct ua_params *params,
				      bool running_in_debugger, bool is_karena,
				      uint64_t iterations, alloc_fn_t alloc_fn,
				      const char *alloc_fn_name,
				      LmString log_filename,
				      const char *file_mode,
				      const char *log_filename_base)
{
	tight_loop_test(params, running_in_debugger, is_karena, iterations,
			alloc_fn, alloc_fn_name, small_sizes,
			LmArrayLen(small_sizes), "small", log_filename,
			file_mode, log_filename_base);

	tight_loop_test(params, running_in_debugger, is_karena, iterations,
			alloc_fn, alloc_fn_name, medium_sizes,
			LmArrayLen(medium_sizes), "medium", log_filename,
			file_mode, log_filename_base);

	tight_loop_test(params, running_in_debugger, is_karena, iterations,
			alloc_fn, alloc_fn_name, large_sizes,
			LmArrayLen(large_sizes), "large", log_filename,
			file_mode, log_filename_base);
}

static int write_tsc_freq_to_file(LmString log_dir, int run_nr)
{
	UAScratch uas = ua_scratch_begin(main_ua);
	LmString tsc_freq_filename = lm_string_make(log_dir, uas.ua);
	lm_string_append_fmt(tsc_freq_filename, "%d-tsc_freq.bin", run_nr);
	double tsc_freq = get_tsc_freq();
	if (lm_write_bytes_to_file_by_name((uint8_t *)&tsc_freq,
					   sizeof(tsc_freq),
					   tsc_freq_filename) != 0)
		return -1;
	ua_scratch_release(uas);
	return 0;
}

static void prepare_logging(cJSON *log_dir_json, LmString *log_dir,
			    LmString *log_filename)
{
	*log_dir = lm_string_make(cJSON_GetStringValue(log_dir_json), main_ua);
	make_dir(*log_dir);
	*log_filename = lm_string_make(*log_dir, main_ua);
	int run_nr = get_next_run_nr(*log_dir);
	lm_string_append_fmt(*log_filename, "%d-log.txt", run_nr);
	write_tsc_freq_to_file(*log_dir, run_nr);
}

// TODO: (isa): Make the data directory just "./logs/arena/", since karena
// is included in the tests and it doesn't make sense to have the ua config
// as the name
static int arena_test(void *ctx, bool running_in_debugger)
{
	cJSON *ctx_json = ctx;
	cJSON *arena_sz_json = cJSON_GetObjectItem(ctx_json, "arena_sz");
	cJSON *mallocd_json = cJSON_GetObjectItem(ctx_json, "mallocd");
	cJSON *contiguous_json = cJSON_GetObjectItem(ctx_json, "contiguous");
	cJSON *alloc_iterations_json =
		cJSON_GetObjectItem(ctx_json, "alloc_iterations");
	cJSON *log_directory_json =
		cJSON_GetObjectItem(ctx_json, "log_directory");
	LmAssert(arena_sz_json && mallocd_json && contiguous_json &&
			 alloc_iterations_json && log_directory_json,
		 "u_arena_test's context JSON is malformed");

	struct ua_params params = { 0 };

	params.arena_sz =
		lm_mem_sz_from_string(cJSON_GetStringValue(arena_sz_json));
	params.mallocd = cJSON_IsTrue(mallocd_json);
	params.contiguous = cJSON_IsTrue(contiguous_json);
	uint64_t alloc_iterations =
		(uint64_t)cJSON_GetNumberValue(alloc_iterations_json);
	LmAssert(alloc_iterations > 0, "u_arena_test's alloc_iterations is 0");

	LmString log_dir;
	LmString log_filename;
	prepare_logging(log_directory_json, &log_dir, &log_filename);

	FILE *log_file = lm_open_file_by_name(log_filename, "w");
	LmSetLogFileLocal(log_file);
	LmLogInfoR("UArena info:\n"
		   "\tContiguous:   %s\n"
		   "\tMallocd:      %s\n"
		   "\tCap:          %zd\n",
		   LmBoolToString(params.contiguous),
		   LmBoolToString(params.mallocd), params.arena_sz);
	LmLogInfoR("\nTSC freq: %.0f\n", get_tsc_freq());
	LmRemoveLogFileLocal();
	lm_close_file(log_file);

	const char *file_mode = "a";
	for (int i = 0; i < (int)LmArrayLen(a_alloc_functions); ++i) {
		alloc_fn_t alloc_fn = a_alloc_functions[i];
		const char *alloc_fn_name = a_alloc_function_names[i];
		realloc_fn_t realloc_fn = a_realloc_functions[0];

		bool is_karena = (alloc_fn == ka_alloc_timed ||
				  alloc_fn == oka_alloc_timed);

		tight_loop_test_all_sizes(&params, running_in_debugger,
					  is_karena, alloc_iterations, alloc_fn,
					  alloc_fn_name, log_filename,
					  file_mode, log_dir);
	}

	return 0;
}

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

	LmString log_dir;
	LmString log_filename;
	prepare_logging(log_directory_json, &log_dir, &log_filename);

	LmAssert(alloc_iterations > 0, "malloc_test's alloc_iterations is 0");

	const char *file_mode = "a";
	for (int i = 0; i < (int)LmArrayLen(malloc_and_fam); ++i) {
		alloc_fn_t alloc_fn = malloc_and_fam[i];
		const char *alloc_fn_name = malloc_and_fam_names[i];
		realloc_fn_t realloc_fn = realloc_functions[0];

		tight_loop_test_all_sizes(NULL, running_in_debugger, false,
					  alloc_iterations, alloc_fn,
					  alloc_fn_name, log_filename,
					  file_mode, log_dir);
	}
	return 0;
}

static int sdhs_test(void *ctx, bool running_in_debugger)
{
	cJSON *ctx_json = ctx;
	cJSON *log_directory_json =
		cJSON_GetObjectItem(ctx_json, "log_directory");
	if (!ctx_json || !log_directory_json) {
		LmLogError(
			"Context JSON or log directory entry for the sdhs test do not exist!");
		return -1;
	}

	LmString log_dir;
	LmString log_filename;
	prepare_logging(log_directory_json, &log_dir, &log_filename);

	if (!running_in_debugger) {
		pid_t pid;
		int status;
		if ((pid = fork()) == -1) {
			LmLogError("Fork failed: %s", strerror(errno));
			return -1;
		} else if (pid == 0) {
			SdhsMain(log_dir);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}
	} else {
		SdhsMain(log_dir);
	}

	return 0;
}

static struct test_definition test_definitions[] = { { arena_test, "arena" },
						     { malloc_test, "malloc" },
						     { sdhs_test, "sdhs" },
						     { 0 } };

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
