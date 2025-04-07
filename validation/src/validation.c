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

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
	lm_file_data *test_config_file =
		lm_load_file_into_memory("./configs/validation.json");
	cJSON *test_config_json = cJSON_Parse((char *)test_config_file->data);
	cJSON *suites_json = cJSON_GetObjectItem(test_config_json, "suites");

	MunitSuite *test_suite = NULL;
	cJSON *suite_json;
	cJSON_ArrayForEach(suite_json, suites_json)
	{
		test_suite = create_munit_suite(suite_json);
	}

	int success = munit_suite_main(test_suite, NULL, argc, argv);
	return success;
}
