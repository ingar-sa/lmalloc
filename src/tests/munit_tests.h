
#include <src/munit/munit.h>
#include <src/cJSON/cJSON.h>

typedef void (*test_debug_fn)(void *test_ctx);

struct test_definition {
	MunitTestFunc test_fn;
	MunitTestSetup setup_fn;
	MunitTestTearDown teardown_fn;
	test_debug_fn debug_fn;
	const char *test_name;
};

MunitSuite *create_munit_suite(cJSON *suite_conf);
MunitTest *get_suite_tests(cJSON *suite_tests_json);
struct test_definition *get_test_definition(cJSON *test_name_json);
