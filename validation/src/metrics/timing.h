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
#endif
