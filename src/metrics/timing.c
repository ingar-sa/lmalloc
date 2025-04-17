#include <src/lm.h>

#include "timing.h"

#include <time.h>

// TODO: (isa): RDTSC timing

long long lm_get_time_stamp(clockid_t type)
{
	struct timespec ts;
	(void)clock_gettime(type, &ts);
	// TODO: (isa): Consider checking return, but gotta go fast though
	long long stamp = LmSToNs(ts.tv_sec) + ts.tv_nsec;
	return stamp;
}

// NOTE: (isa): "Filled out" by Claude
static void lm_format_timing(long long timing, enum time_stamp_fmt stamp_fmt,
			     char buf[64])
{
	switch (stamp_fmt) {
	case S_NS:
		snprintf(buf, 64, "%lld.%09lld s", LmNsToS(timing),
			 timing % 1000000000LL);
		break;
	case S_US:
		snprintf(buf, 64, "%lld.%06lld s", LmNsToS(timing),
			 (timing % 1000000000LL) / 1000LL);
		break;
	case S_MS:
		snprintf(buf, 64, "%lld.%03lld s", LmNsToS(timing),
			 (timing % 1000000000LL) / 1000000LL);
		break;
	case US_TRUNK:
		snprintf(buf, 64, "%lld us", timing / 1000LL);
		break;
	case MS_TRUNK:
		snprintf(buf, 64, "%lld ms", timing / 1000000LL);
		break;
	case S_TRUNK:
		snprintf(buf, 64, "%lld s", timing / 1000000000LL);
		break;
	case NS:
		snprintf(buf, 64, "%lld ns", timing);
		break;
	case US:
		snprintf(buf, 64, "%lld.%03lld us", timing / 1000LL,
			 timing % 1000LL);
		break;
	case MS:
		snprintf(buf, 64, "%lld.%06lld ms", timing / 1000000LL,
			 timing % 1000000LL);
		break;
	case S:
		snprintf(buf, 64, "%lld.%09lld s", LmNsToS(timing),
			 timing % 1000000000LL);
		break;
	default:
		snprintf(buf, 64, "Unknown format");
		break;
	}
}

void lm_print_timing(long long timing, const char *description,
		     enum time_stamp_fmt stamp_fmt)
{
	char timing_str[64];
	lm_format_timing(timing, stamp_fmt, timing_str);
	printf("%s: %s\n", description, timing_str);
}

// TODO: (isa): The compiler probably can't optimize away the call to this function
// since the log_level isn't available at compile time. We should probably make a
// macro wrapper for the function call
void lm_log_timing(long long timing, const char *description,
		   enum time_stamp_fmt stamp_fmt, bool log_raw,
		   enum lm_log_level log_level, lm_log_module *log_module)
{
	char timing_str[64];
	lm_format_timing(timing, stamp_fmt, timing_str);
	LmLogManual(log_module, log_raw, log_level, "%s: %s", description,
		    timing_str);
}

void lm_print_timing_avg(long long timing, long long count,
			 const char *description, enum time_stamp_fmt stamp_fmt)
{
	char timing_str[64];
	double avg = (double)timing / (double)count;
	enum time_stamp_fmt adjusted_fmt = stamp_fmt;

	if (avg < 1000.0 && stamp_fmt != NS) {
		adjusted_fmt = NS;
	} else if (avg < 1000000.0 && stamp_fmt != NS && stamp_fmt != US) {
		adjusted_fmt = US;
	}

	long long avg_ll = (long long)avg;
	lm_format_timing(avg_ll, adjusted_fmt, timing_str);

	printf("%s: %s (total: %lld ns, count: %lld)", description, timing_str,
	       timing, count);
}

// NOTE: (isa): Claude rewrote to this version
void lm_log_timing_avg(long long timing, long long count,
		       const char *description, enum time_stamp_fmt stamp_fmt,
		       bool log_raw, enum lm_log_level log_level,
		       lm_log_module *log_module)
{
	char timing_str[64];
	double avg = (double)timing / (double)count;
	enum time_stamp_fmt adjusted_fmt = stamp_fmt;

	if (avg < 1000.0 && stamp_fmt != NS) {
		adjusted_fmt = NS;
	} else if (avg < 1000000.0 && stamp_fmt != NS && stamp_fmt != US) {
		adjusted_fmt = US;
	}

	long long avg_ll = (long long)avg;
	lm_format_timing(avg_ll, adjusted_fmt, timing_str);

	LmLogManual(log_module, log_raw, log_level,
		    "%s%s (total: %lld ns, count: %lld)", description,
		    timing_str, timing, count);
}

// NOTE: (isa): Original written by claude
int lm_compare_timing(long long t1, long long t2, struct timing_comp *tc)
{
	if (t1 == t2) {
		printf("Both timestamps are identical (0%% difference, 1.00x)\n");
		tc->percentage = 0.0;
		tc->multiplier = 1.0;
		return 0;
	}

	if (t1 < t2) {
		tc->percentage = ((double)(t2 - t1) / (double)t2) * 100.0;
		tc->multiplier = (double)t2 / (double)t1;
		return 1;
	} else {
		tc->percentage = ((double)(t1 - t2) / (double)t2) * 100.0;
		tc->multiplier = (double)t1 / (double)t2;
		return -1;
	}
}

void lm_print_timing_comp(struct timing_comp tc, int res)
{
	if (res == 0) {
		printf("Both timestamps are identical (0%% difference, 1.00x)\n");
	} else if (res > 0) {
		printf("t1 is faster than t2 by %.2f%% (%.2fx faster)\n",
		       tc.percentage, tc.multiplier);
	} else {
		printf("t1 is slower than t2 by %.2f%% (%.2fx slower)\n",
		       tc.percentage, tc.multiplier);
	}
}

// TODO: (isa): Same as with the time logging: make a wrapper macro
void lm_log_timing_comp(struct timing_comp tc, int res, bool log_raw,
			enum lm_log_level log_level, lm_log_module *log_module)
{
	if (res == 0) {
		LmLogManual(
			log_module, log_raw, log_level,
			"Both timestamps are identical (0%% difference, 1.00x)\n");
	} else if (res > 0) {
		LmLogManual(log_module, log_raw, log_level,
			    "t1 is faster than t2 by %.2f%% (%.2fx faster)\n",
			    tc.percentage, tc.multiplier);
	} else {
		LmLogManual(log_module, log_raw, log_level,
			    "t1 is slower than t2 by %.2f%% (%.2fx slower)\n",
			    tc.percentage, tc.multiplier);
	}
}
