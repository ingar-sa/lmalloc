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

int main(int argc, char **argv)
{
	int result = EXIT_SUCCESS;
#if 0
	// NOTE: (isa): This #if/else is just a quick way to run debug stuff instead of the main code

	//LmLogDebug("Sizeof UArena: %zd", sizeof(UArena));
	SdhsMain(0, NULL);
	//LmLogDebug("TSC freq: %f", calibrate_tsc() / 1e9);
	//LmLogDebug("CPU has invariant tsc: %s",
	//	   LmBoolToString(cpu_has_invariant_tsc()));
#else
	size_t cjson_ua_sz = LmKibiByte(16);
	cjson_arena = ua_create(cjson_ua_sz, true, false, 16);
	cJSON_Hooks cjson_hooks = { 0 };
	cjson_hooks.malloc_fn = cjson_alloc;
	cjson_hooks.free_fn = cjson_free;
	cJSON_InitHooks(&cjson_hooks);

	size_t main_ua_sz = LmMebiByte(1);
	UArena *main_ua = ua_create(main_ua_sz, true, false, 16);

	lm_file_data *test_config_file =
		lm_load_file_into_memory("./configs/validation.json", main_ua);
	cJSON *test_config_json = cJSON_Parse((char *)test_config_file->data);
	result = run_tests(test_config_json);
#endif
	return result;
}
