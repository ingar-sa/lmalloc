#define LM_H_IMPLEMENTATION
#include <src/lm.h>
#undef LM_H_IMPLEMENTATION

LM_LOG_GLOBAL_REGISTER();
LM_LOG_REGISTER(validation);

#include <src/metrics/timing.h>
#include <src/tests/allocator_tests.h>
#include <src/allocators/u_arena.h>

#include <src/tests/munit_tests.h>

#include <src/munit/munit.h>

#include <stdlib.h>
#include <stddef.h>

#ifndef RUN_IN_DEBUGGER
#define RUN_IN_DEBUGGER false
#endif

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
	int success = EXIT_SUCCESS;

	if (!RUN_IN_DEBUGGER) {
		lm_file_data *test_config_file =
			lm_load_file_into_memory("./configs/validation.json");
		cJSON *test_config_json =
			cJSON_Parse((char *)test_config_file->data);
		cJSON *main_suite_json =
			cJSON_GetObjectItem(test_config_json, "main_suite");

		MunitSuite *main_suite = create_munit_suite(main_suite_json);
		if (main_suite)
			success =
				munit_suite_main(main_suite, NULL, argc, argv);
		else
			LmLogInfo("Test suite disabled!");
	} else {
		malloc_tests_debugger();
	}

	return success;
}
