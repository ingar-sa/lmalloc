#include <src/lm.h>
LM_LOG_REGISTER(tight_loop_test);

#include <src/allocators/allocator_wrappers.h>
#include <src/metrics/timing.h>

#include "tight_loop_test.h"
#include "tests.h"

#include <stddef.h>
#include <stdlib.h>
#include <sys/wait.h>

extern UArena *main_ua;

static enum allocation_type get_allocation_type(alloc_fn_t alloc_fn)
{
	enum allocation_type type = -1;
	if (alloc_fn == ua_alloc_wrapper_timed)
		type = UA_ALLOC;
	else if (alloc_fn == ua_zalloc_wrapper_timed)
		type = UA_ZALLOC;
	else if (alloc_fn == ua_falloc_wrapper_timed)
		type = UA_FALLOC;
	else if (alloc_fn == ua_fzalloc_wrapper_timed)
		type = UA_FZALLOC;
	else if (alloc_fn == malloc_wrapper_timed)
		type = MALLOC;
	else if (alloc_fn == calloc_wrapper_timed)
		type = CALLOC;

	return type;
}

static void all_sizes_repeatedly(UArena *test_ua, uint64_t alloc_iterations,
				 alloc_fn_t alloc_fn, const char *alloc_fn_name,
				 size_t *alloc_sizes, size_t alloc_sizes_len,
				 const char *size_name,
				 const char *log_directory)
{
	LmLogInfoR("\n\n%s'ing all %s sizes repeatedly %lu times: \n",
		   alloc_fn_name, size_name, alloc_iterations);
	uint64_t total_iterations = alloc_iterations * alloc_sizes_len;
	size_t timing_vals_sz = total_iterations * sizeof(uint64_t);
	UArena *timings_ua = ua_create(timing_vals_sz, UA_CONTIGUOUS, UA_MMAPD,
				       sizeof(uint64_t));
	uint64_t *timing_arr =
		UaPushArray(timings_ua, uint64_t, total_iterations);
	provide_alloc_timing_collection_arr(total_iterations, timing_arr);
	struct alloc_timing_collection *timings = get_wrapper_timings();
	for (size_t i = 0; i < alloc_iterations; ++i) {
		for (uint j = 0; j < alloc_sizes_len; ++j) {
			uint8_t *ptr = alloc_fn(test_ua, alloc_sizes[j]);
			if (!!0 && LM_UNLIKELY(!ptr)) {
				LmLogDebug("Arena ran out of memory! Freeing");
				ua_free(test_ua);
				timings->idx -=
					1; // Overwrite failed allocation timing
				ptr = alloc_fn(test_ua, alloc_sizes[j]);
			}
			*ptr = 1;
		}
	}

	if (test_ua)
		ua_free(test_ua);

	enum allocation_type alloc_type = get_allocation_type(alloc_fn);
	log_allocation_timing_avg(alloc_type, get_alloc_stats(), "", NS, true,
				  INF, LM_LOG_MODULE_LOCAL);
	LmLogInfoR("\n");

	UAScratch uas = ua_scratch_begin(main_ua);

	LmString data_dump_filename = lm_string_make(log_directory, uas.ua);
	lm_string_append_fmt(data_dump_filename, "%s-%s.bin",
			     alloct_string(alloc_type), size_name);
	if (write_timing_data_to_file(data_dump_filename, alloc_type) != 0) {
		LmLogError("Failed to write data to file %s",
			   data_dump_filename);
		return;
	}

	ua_scratch_release(uas);
	ua_destroy(&timings_ua);
	clear_alloc_timing_stats(alloc_type);
	clear_wrapper_alloc_timing_collection();
}

// NOTE: (isa): If the memory is freed, then malloc will
// use just as little time as the arena for all allocations,
// likely due to it simply reusing the same slot in the free list.
// It could be interesting to have this as its own test.
static void each_size_by_itself(UArena *test_ua, uint64_t alloc_iterations,
				alloc_fn_t alloc_fn, const char *alloc_fn_name,
				size_t *alloc_sizes, size_t alloc_sizes_len,
				const char *size_name,
				const char *log_directory)
{
	LmLogInfoR("\n%s'ing each %s size %lu times\n", alloc_fn_name,
		   size_name, alloc_iterations);

	size_t timing_vals_sz = alloc_iterations * sizeof(uint64_t);
	UArena *timings_ua = ua_create(timing_vals_sz, UA_CONTIGUOUS, UA_MMAPD,
				       sizeof(uint64_t));
	uint64_t *timing_arr =
		UaPushArray(timings_ua, uint64_t, alloc_iterations);
	provide_alloc_timing_collection_arr(alloc_iterations, timing_arr);
	struct alloc_timing_collection *timings = get_wrapper_timings();

	for (size_t j = 0; j < alloc_sizes_len; ++j) {
		LmLogInfoR("\n%zd bytes: \n", alloc_sizes[j]);

		for (uint64_t i = 0; i < alloc_iterations; ++i) {
			uint8_t *ptr = alloc_fn(test_ua, alloc_sizes[j]);
			if (!!0 && LM_UNLIKELY(!ptr)) {
				LmLogDebug("Arena ran out of memory! Freeing");
				ua_free(test_ua);
				timings->idx -=
					1; // Overwrite failed allocation timing
				ptr = alloc_fn(test_ua, alloc_sizes[j]);
			}
			*ptr = 1;
		}

		ua_free(test_ua);

		struct alloc_timing_stats *stats = get_alloc_stats();
		enum allocation_type alloc_type = get_allocation_type(alloc_fn);
		log_allocation_timing_avg(alloc_type, stats, "", NS, true, INF,
					  LM_LOG_MODULE_LOCAL);
		LmLogInfoR("\n");

		UAScratch uas = ua_scratch_begin(main_ua);

		LmString data_dump_filename =
			lm_string_make(log_directory, uas.ua);
		lm_string_append_fmt(data_dump_filename, "%s-%zdB.bin",
				     alloct_string(alloc_type), alloc_sizes[j]);
		if (write_timing_data_to_file(data_dump_filename, alloc_type) !=
		    0) {
			LmLogError("Failed to write data to file %s",
				   data_dump_filename);
			return;
		}

		ua_scratch_release(uas);
		clear_alloc_timing_stats(alloc_type);
		timings->idx = 0;
	}

	ua_destroy(&timings_ua);
	clear_wrapper_alloc_timing_collection();
}

void tight_loop_test(struct ua_params *ua_params, bool running_in_debugger,
		     uint64_t alloc_iterations, alloc_fn_t alloc_fn,
		     const char *alloc_fn_name, size_t *alloc_sizes,
		     size_t alloc_sizes_len, const char *size_name,
		     LmString log_filename, const char *file_mode,
		     const char *log_directory)
{
	if (ua_params) {
		size_t largest_sz = alloc_sizes[alloc_sizes_len - 1];
		size_t mem_needed_for_largest_sz =
			largest_sz * (size_t)alloc_iterations;
		LmAssert(
			ua_params->arena_sz >= mem_needed_for_largest_sz,
			"Arena has insufficient memory for %s sizes allocation test. Arena size: %zd, needed size: %zd",
			size_name, ua_params->arena_sz,
			mem_needed_for_largest_sz);
	}

	UAScratch uas = ua_scratch_begin(main_ua);
	if (!running_in_debugger) {
		pid_t pid;
		int status;
		if ((pid = fork()) == -1) {
			LmLogError("Fork failed: %s", strerror(errno));
		} else if (pid == 0) {
			FILE *log_file =
				lm_open_file_by_name(log_filename, file_mode);
			LmSetLogFileLocal(log_file);

			UArena *ua = NULL;
			if (ua_params)
				ua = ua_create(ua_params->arena_sz,
					       ua_params->contiguous,
					       ua_params->mallocd,
					       ua_params->alignment);

			LmLogInfoR("\n\n------------------------------\n");
			LmLogInfo("%s -- %s", alloc_fn_name, size_name);

			each_size_by_itself(ua, alloc_iterations, alloc_fn,
					    alloc_fn_name, alloc_sizes,
					    alloc_sizes_len, size_name,
					    log_directory);
			if (ua)
				ua_destroy(&ua);
			LmRemoveLogFileLocal();
			lm_close_file(log_file);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}

		if ((pid = fork()) == -1) {
			LmLogError("Fork failed: %s", strerror(errno));
		} else if (pid == 0) {
			FILE *log_file =
				lm_open_file_by_name(log_filename, file_mode);
			LmSetLogFileLocal(log_file);

			UArena *ua = NULL;
			if (ua_params)
				ua = ua_create(ua_params->arena_sz,
					       ua_params->contiguous,
					       ua_params->mallocd,
					       ua_params->alignment);

			all_sizes_repeatedly(ua, alloc_iterations, alloc_fn,
					     alloc_fn_name, alloc_sizes,
					     alloc_sizes_len, size_name,
					     log_directory);
			if (ua)
				ua_destroy(&ua);
			LmRemoveLogFileLocal();
			lm_close_file(log_file);
			exit(EXIT_SUCCESS);
		} else {
			waitpid(pid, &status, 0);
		}
	} else {
		FILE *log_file = lm_open_file_by_name(log_filename, file_mode);
		LmSetLogFileLocal(log_file);
		static UArena *ua = NULL;
		if (!ua && ua_params)
			ua = ua_create(ua_params->arena_sz,
				       ua_params->contiguous,
				       ua_params->mallocd,
				       ua_params->alignment);

		LmLogInfoR("\n\n------------------------------\n");
		LmLogInfo("%s -- %s", alloc_fn_name, size_name);

		each_size_by_itself(ua, alloc_iterations, alloc_fn,
				    alloc_fn_name, alloc_sizes, alloc_sizes_len,
				    size_name, log_directory);

		all_sizes_repeatedly(ua, alloc_iterations, alloc_fn,
				     alloc_fn_name, alloc_sizes,
				     alloc_sizes_len, size_name, log_directory);
		if (ua)
			ua_destroy(&ua);

		LmRemoveLogFileLocal();
		lm_close_file(log_file);
	}

	ua_scratch_release(uas);
}
