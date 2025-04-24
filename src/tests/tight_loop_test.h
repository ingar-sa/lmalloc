#ifndef TIGHT_LOOP_TEST_H
#define TIGHT_LOOP_TEST_H

#include <src/lm.h>

#include <src/allocators/u_arena.h>
#include <src/allocators/allocator_wrappers.h>

#include "tests.h"

void tight_loop_test(struct ua_params *ua_params, bool running_in_debugger,
		     uint64_t alloc_iterations, alloc_fn_t alloc_fn,
		     const char *alloc_fn_name, size_t *alloc_sizes,
		     size_t alloc_sizes_len, const char *size_name,
		     const char *log_filename, const char *file_mode);

#endif
