
#include <src/munit/munit.h>
#include <src/cJSON/cJSON.h>

typedef MunitResult (*munit_test_fn)(const MunitParameter params[], void *data);
typedef void *(*munit_setup_fn)(const MunitParameter params[], void *data);
typedef void (*munit_teardown_fn)(void *fixture);

MunitSuite *create_munit_suite(cJSON *suite_conf);
