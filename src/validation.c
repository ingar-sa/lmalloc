#define LM_H_IMPLEMENTATION
#include <src/lm.h>
#undef LM_H_IMPLEMENTATION

LM_LOG_GLOBAL_REGISTER();
LM_LOG_REGISTER(validation);

#include <src/metrics/timing.h>
#include <src/tests/test_setup.h>
#include <src/allocators/u_arena.h>

#include <src/munit/munit.h>

#include <stdlib.h>
#include <stddef.h>

// NOTE: (isa): We are pretty much not freeing any of the memory used by cJSON
// or munit atm, but that memory can live happily for the entire lifetime of
// the program, so it's not an issue (famous last words, I know).

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
	int result = EXIT_SUCCESS;

	lm_file_data *test_config_file =
		lm_load_file_into_memory("./configs/validation.json");
	cJSON *test_config_json = cJSON_Parse((char *)test_config_file->data);
	cJSON *main_suite_json =
		cJSON_GetObjectItem(test_config_json, "main_suite");
	LmAssert(main_suite_json != NULL,
		 "main_suite not present in validation.json");

	MunitSuite *main_suite = create_munit_suite(main_suite_json);
	if (!main_suite) {
		LmLogWarning("Main suite is disabled!");
		return result;
	}

	if (!main_suite->running_in_debugger) {
		// TODO: (isa): See if we can pass in the flag for not forking
		result =
			munit_suite_main(main_suite, (void *)false, argc, argv);
	} else {
		for (MunitTest *test = main_suite->tests; test->test != NULL;
		     test++) {
			if (test->setup == NULL) {
				test->test(NULL, (void *)true);
			} else {
				void *test_ctx = test->setup(NULL, (void *)true,
							     test->ctx);
				test->test(NULL, test_ctx);
			}
		}
	}

	return result;
}
