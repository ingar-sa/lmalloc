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
	void *ptr = ua_alloc(cjson_arena, sz);
	if (!ptr) {
		LmLogError("Insufficient memory for cJSON allocation");
		exit(EXIT_FAILURE);
	}
	return ptr;
}

// NOTE: (isa): This is only called in functions that change/add things to JSON objects,
// which we don't do, so not freeing shouldn't be a problem.
void cjson_free(void *ptr)
{
	(void)ptr;
}

int main(int argc, char **argv)
{
	int result = EXIT_SUCCESS;

	size_t main_ua_sz = LmGibiByte(4);
	main_ua = ua_create(main_ua_sz, UA_CONTIGUOUS, UA_MMAPD, 16);

	size_t cjson_ua_sz = LmKibiByte(512);
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
	size_t config_file_sz = 0;
	uint8_t *test_config_file = lm_load_file_into_memory(
		"./configs/validation.json", &config_file_sz, main_ua);
	cJSON *test_config_json = cJSON_Parse((char *)test_config_file);
	result = run_tests(test_config_json);
#endif
	return result;
}
