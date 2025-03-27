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

long long lm_get_time_stamp(clockid_t type);

void lm_print_timing(long long start, long long end, const char *description,
		     enum time_stamp_fmt stamp_fmt);

void lm_log_timing(long long start, long long end, const char *description,
		   enum time_stamp_fmt stamp_fmt, enum lm_log_level log_level,
		   lm_log_module *log_module);
#endif
