#include <src/lm.h>

LM_LOG_GLOBAL_DECLARE();
LM_LOG_REGISTER(system_info);

#include <unistd.h>
#include <errno.h>

#include <src/metrics/timing.h>

#include "system_info.h"

// NOTE: (isa): Written by Claude
bool cpu_has_invariant_tsc(void)
{
	uint32_t eax, ebx, ecx, edx;
	__asm__ volatile("cpuid"
			 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
			 : "a"(0x80000007), "c"(0));

	return !!(edx & (1 << 8));
}

// NOTE: (isa): Written by Claude
double get_tsc_freq(void)
{
	if (!cpu_has_invariant_tsc())
		LmLogWarning(
			"The CPU does not have an invariant TSC. This means that "
			"it will increase variably with changes in clock frequency, "
			"and comparisons between readings during execution with different "
			"frequencies are meaningless. Consider using lm_get_time_stamp "
			"instead.");

	static double tsc_freq = 0.0;
	static bool tsc_has_been_calibrated = false;
	if (!tsc_has_been_calibrated) {
		struct timespec start_ts, end_ts;
		double elapsed_sec, cycles_per_sec;

		clock_gettime(CLOCK_MONOTONIC, &start_ts);
		START_TSC_TIMING_LFENCE(tsc);

		usleep(100000);

		END_TSC_TIMING_LFENCE(tsc);
		clock_gettime(CLOCK_MONOTONIC, &end_ts);

		elapsed_sec = (double)(end_ts.tv_sec - start_ts.tv_sec) +
			      (double)(end_ts.tv_nsec - start_ts.tv_nsec) / 1e9;

		cycles_per_sec = (double)(tsc_end - tsc_start) / elapsed_sec;
		tsc_freq = cycles_per_sec;
		tsc_has_been_calibrated = true;
	}

	return tsc_freq;
}

// NOTE: Original written by claude
size_t get_page_size(void)
{
	long page_size = sysconf(_SC_PAGESIZE);
	if (page_size == -1) {
		if (errno == 0)
			LmAssert(0, "Page size is indeterminate");
		else
			LmAssert(0, "Failed to get page size (%s)",
				 strerror(errno));
	}

	return (size_t)page_size;
}

size_t get_l1d_cacheln_sz(void)
{
	long l1d_cache_line = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
	LmAssert(l1d_cache_line > 0,
		 "L1 cache line size returned from sysconf was <= 0");
	return (size_t)l1d_cache_line;
}
