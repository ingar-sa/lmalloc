#define LM_H_IMPLEMENTATION
#include <src/lm.h>
#undef LM_H_IMPLEMENTATION

LM_LOG_GLOBAL_REGISTER();
LM_LOG_REGISTER(validation);

#include <src/metrics/timing.h>
#include <src/allocators/u_arena.h>
#include <src/tests/tests.h>
#include <src/utils/system_info.h>
#include <src/sdhs/Sdhs.h>

#include <src/cJSON/cJSON.h>

#include <stdlib.h>
#include <stddef.h>

UArena *main_ua;

void *cjson_alloc(size_t);
void cjson_free(void *ptr);

// NOTE: (isa): Separate arena for cjson so it does not use malloc, which could affect our tests
static UArena *cjson_arena;
void *cjson_alloc(size_t sz)
{
	return ua_alloc(cjson_arena, sz);
}

// NOTE: (isa): This is only called in functions that change/add things to JSON objects,
// which we don't do, so not freeing shouldn't be a problem.
void cjson_free(void *ptr)
{
	(void)ptr;
}

struct program_args {
	int cpu_core;
};

static void print_use(void)
{
	printf("\tArguments that must be passed to the program:\n"
	       "\t-core=<core number> where 'core number' is the number specified to 'taskset'\n");
}

// NOTE: (isa): Written by Claude
static void parse_args(int argc, char **argv, struct program_args *args)
{
	if (argc <= 1) {
		printf("\tNo parameters were given\n");
		print_use();
		exit(EXIT_FAILURE);
	}

	args->cpu_core = -1;

	for (int i = 1; i < argc; ++i) {
		if (strncmp(argv[i], "-core=", 6) == 0) {
			char *value = argv[i] + 6;
			if (*value == '\0') {
				fprintf(stderr,
					"Error: No value provided for -core argument\n");
				exit(EXIT_FAILURE);
			}
			args->cpu_core = atoi(value);
		}
	}

	LmAssert(args->cpu_core >= 0,
		 "No CPU core number or a negative value were provided");
	printf("Cpu core: %d\n", args->cpu_core);
}

// TODO: (isa): Log which cpu core the process is being run on
int main(int argc, char **argv)
{
	int result = EXIT_SUCCESS;

	struct program_args args;
	parse_args(argc, argv, &args);

	size_t main_ua_sz = LmGibiByte(64);
	main_ua = ua_create(main_ua_sz, true, false, 16);

	size_t cjson_ua_sz = LmKibiByte(16);
	cjson_arena =
		ua_bootstrap(main_ua, NULL, cjson_ua_sz, main_ua->alignment);
	cJSON_Hooks cjson_hooks = { 0 };
	cjson_hooks.malloc_fn = cjson_alloc;
	cjson_hooks.free_fn = cjson_free;
	cJSON_InitHooks(&cjson_hooks);

#if 0
	// NOTE: (isa): This #if/else is just a quick way to run debug stuff instead of the main code

	LmLogDebugR("%s", ua_info_string(cjson_arena, main_ua));

#else
	lm_file_data *test_config_file =
		lm_load_file_into_memory("./configs/validation.json", main_ua);
	cJSON *test_config_json = cJSON_Parse((char *)test_config_file->data);
	result = run_tests(test_config_json);
#endif
	return result;
}
