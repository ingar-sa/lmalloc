#ifndef LM_TIMING_H
#define LM_TIMING_H

#include <src/lm.h>

#include <time.h>

#define LmSToNs(s) (s * 1000000000LL)
#define LmSToUs(s) (s * 1000000LL)
#define LmSToMs(s) (s * 1000LL)

#define LmNsToS(s) (s / 1000000000LL)
#define LmUsToS(s) (s / 1000000LL)
#define LmMsToS(s) (s / 1000LL)

// TODO: (isa): Are these defines really necessary?
#define MONOTONIC CLOCK_MONOTONIC
#define MONOTONIC_RAW CLOCK_MONOTONIC_RAW
#define REALTIME CLOCK_REALTIME
#define PROC_CPUTIME CLOCK_PROCESS_CPUTIME_ID
#define THREAD_CPUTIME CLOCK_THREAD_CPUTIME_ID

enum time_stamp_fmt {
	S_NS,
	S_MS,
	S_US,
	US_TRUNK,
	MS_TRUNK,
	S_TRUNK,
	NS,
	US,
	MS,
	S,
};

struct timing_comp {
	double percentage;
	double multiplier;
};

struct timing_interval {
	long long start;
	long long end;
};

static const char *lm_clock_type_str(clockid_t type)
{
	switch (type) {
	case MONOTONIC:
		return "MONOTONIC";
		break;
	case MONOTONIC_RAW:
		return "MONOTONIC RAW";
		break;
	case REALTIME:
		return "REALTIME";
		break;
	case PROC_CPUTIME:
		return "PROC CPUTIME";
		break;
	case THREAD_CPUTIME:
		return "THREAD CPUTIME";
		break;
	default:
		return "Unknown clock type";
		break;
	}
}

#define LM_START_TIMING(name, type) \
	long long name##_start = lm_get_time_stamp(type)

#define LM_END_TIMING(name, type) long long name##_end = lm_get_time_stamp(type)

// NOTE: (isa): There seems to be a consistent pattern that the smallest "unit"
// of a TSC timing is 10^(-8) * TSC frequency. On my desktop PC, which has a
// 3.8GHz base clock speed with an invariant TSC -- meaning the TSC will always
// increase at a fixed rate regardless of the actual clock speed the CPU is running
// at -- all TSC results will be multiples of 38 TSC. On my laptop, with a base
// clock rate of 2.3GHz, it's multiples of 23. According to the AMD Architecture
// Programmer's Manual Vol. 2 (p. 422),
//      "In general, the TSC should not be used to take very short
//      time measurements, because the resulting measurement is
//      not guaranteed to be identical each time it is made."
// So yeah... maybe using the TSC to time individual allocations is a no go.
// I'll have to investigate further, and potentially look for other ways of
// timing small sections of code. The uops.info people might have a way of doing
// so.
// NOTE: (isa): This is apparently only the case on AMD CPUs. Intel's TSC counter
// works as expected, and we will therefore run benchmarks on an Intel CPU

// NOTE: (isa): The lfence, rdtsc, lfence sequence is used by the uops.info guys
// in the nanoBench software, so we can use that as an argument for why we
// use this method
#define START_TSC_TIMING_LFENCE(name)                              \
	uint64_t name##_start;                                     \
	do {                                                       \
		uint32_t low, high;                                \
		__asm__ volatile("lfence");                        \
		__asm__ volatile("rdtsc" : "=a"(low), "=d"(high)); \
		__asm__ volatile("lfence");                        \
		name##_start = ((uint64_t)high << 32) | low;       \
	} while (0)

#define END_TSC_TIMING_LFENCE(name)                                \
	uint64_t name##_end;                                       \
	do {                                                       \
		uint32_t low, high;                                \
		__asm__ volatile("lfence");                        \
		__asm__ volatile("rdtsc" : "=a"(low), "=d"(high)); \
		__asm__ volatile("lfence");                        \
		name##_end = ((uint64_t)high << 32) | low;         \
	} while (0)

#define START_TSC_TIMING(name)                                                \
	uint64_t name##_start;                                                \
	do {                                                                  \
		uint32_t eax, ebx, ecx, edx;                                  \
		__asm__ volatile("cpuid"                                      \
				 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) \
				 : "a"(0));                                   \
		uint32_t low, high;                                           \
		__asm__ volatile("rdtsc" : "=a"(low), "=d"(high));            \
		name##_start = ((uint64_t)high << 32) | low;                  \
	} while (0)

#define END_TSC_TIMING(name)                                                   \
	uint64_t name##_end;                                                   \
	do {                                                                   \
		uint32_t low, high, aux;                                       \
		__asm__ volatile("rdtscp" : "=a"(low), "=d"(high), "=c"(aux)); \
		name##_end = ((uint64_t)high << 32) | low;                     \
		uint32_t eax, ebx, ecx, edx;                                   \
		__asm__ volatile("cpuid"                                       \
				 : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)  \
				 : "a"(0));                                    \
	} while (0)

#define TscToNs(tsc, freq) ((uint64_t)((double)(tsc) * 1e9 / (freq)))
#define TscToS(tsc, freq) ((uint64_t)((double)(tsc) / (freq)))

double calibrate_tsc(void);

double tsc_to_sec(uint64_t tsc);

long long lm_get_time_stamp(clockid_t type);

void lm_print_timing(long long timing, const char *description,
		     enum time_stamp_fmt stamp_fmt);

void lm_log_timing(long long timing, const char *description,
		   enum time_stamp_fmt stamp_fmt, bool log_raw,
		   enum lm_log_level log_level, lm_log_module *log_module);

#define LM_LOG_TIMING(name, description, stamp_fmt, log_raw, log_level,    \
		      log_module)                                          \
	lm_log_timing((name##_end - name##_start), description, stamp_fmt, \
		      log_raw, log_level, log_module)

void lm_print_timing_avg(long long timing, long long count,
			 const char *description,
			 enum time_stamp_fmt stamp_fmt);

void lm_log_timing_avg(long long timing, long long count,
		       const char *description, enum time_stamp_fmt stamp_fmt,
		       bool log_raw, enum lm_log_level log_level,
		       lm_log_module *log_module);

#define LM_LOG_TIMING_AVG(name, count, description, stamp_fmt, log_raw,    \
			  log_level, log_module)                           \
	lm_log_timing_avg((name##_end - name##_start), count, description, \
			  stamp_fmt, log_raw, log_level, log_module)

int lm_compare_timing(long long t1, long long t2, struct timing_comp *tc);

void lm_print_timing_comp(struct timing_comp tc, int res);

void lm_log_timing_comp(struct timing_comp tc, int res, bool log_raw,
			enum lm_log_level log_level, lm_log_module *log_module);

void lm_log_tsc_timing(uint64_t tsc, const char *description,
		       enum time_stamp_fmt stamp_fmt, bool log_raw,
		       enum lm_log_level log_level, lm_log_module *log_module);

void lm_print_tsc_timing_avg(uint64_t timing, uint64_t count,
			     const char *description,
			     enum time_stamp_fmt stamp_fmt);

void lm_log_tsc_timing_avg(uint64_t tsc_timing, uint64_t count,
			   const char *description,
			   enum time_stamp_fmt stamp_fmt, bool log_raw,
			   enum lm_log_level log_level,
			   lm_log_module *log_module);

#define LM_LOG_TSC_TIMING_AVG(name, count, description, stamp_fmt, log_raw,    \
			      log_level, log_module)                           \
	lm_log_tsc_timing_avg((name##_end - name##_start), count, description, \
			      stamp_fmt, log_raw, log_level, log_module)

int lm_compare_tsc_timing(uint64_t t1, uint64_t t2, struct timing_comp *tc);

void lm_print_tsc_timing_comp(struct timing_comp tc, int res);

void lm_log_tsc_timing_comp(struct timing_comp tc, int res, bool log_raw,
			    enum lm_log_level log_level,
			    lm_log_module *log_module);
#endif
