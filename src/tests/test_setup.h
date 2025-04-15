#ifndef ALLOCATOR_TESTS_H
#define ALLOCATOR_TESTS_H

#include <src/lm.h>

#include <src/munit/munit.h>
#include <src/cJSON/cJSON.h>

#include <stdbool.h>

struct test_definition {
	MunitTestFunc test_fn;
	MunitTestSetup setup_fn;
	MunitTestTearDown teardown_fn;
	const char *test_name;
};

MunitSuite *create_munit_suite(cJSON *suite_conf);
MunitTest *get_suite_tests(cJSON *suite_tests_json);
struct test_definition *get_test_definition(cJSON *test_name_json);

struct u_arena_test_params {
	size_t arena_sz;
	size_t alignment;
	bool contiguous;
	bool mallocd;
	uint alloc_iterations;
	LmString log_filename;
	bool running_in_debugger;
};

struct malloc_test_params {
	uint alloc_iterations;
	LmString log_filename;
	bool running_in_debugger;
};

int u_arena_tests(struct u_arena_test_params *params);
int malloc_tests(struct malloc_test_params *param);

#endif
