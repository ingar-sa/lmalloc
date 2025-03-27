#include <src/lm.h>

#include "timing.h"

#include <time.h>

long long lm_get_time_stamp(clockid_t type)
{
	struct timespec ts;
	(void)clock_gettime(type, &ts);
	// TODO: (isa): Consider checking return, but gotta go fast though
	long long stamp = LmSToNs(ts.tv_sec) + ts.tv_nsec;
	return stamp;
}

// NOTE: (isa): Claude filled out the switch statement
void lm_print_timing(long long start, long long end, const char *description,
		     enum time_stamp_fmt stamp_fmt)
{
	long long diff = end - start;
	switch (stamp_fmt) {
	case S_NS:
		printf("%s: %lld.%09lld s\n", description, LmNsToS(diff),
		       diff % 1000000000LL);
		break;
	case S_US:
		printf("%s: %lld.%06lld s\n", description, LmNsToS(diff),
		       (diff % 1000000000LL) / 1000LL);
		break;
	case S_MS:
		printf("%s: %lld.%03lld s\n", description, LmNsToS(diff),
		       (diff % 1000000000LL) / 1000000LL);
		break;
	case US_TRUNK:
		printf("%s: %lld us\n", description, diff / 1000LL);
		break;
	case MS_TRUNK:
		printf("%s: %lld ms\n", description, diff / 1000000LL);
		break;
	case S_TRUNK:
		printf("%s: %lld s\n", description, diff / 1000000000LL);
		break;
	case NS:
		printf("%s: %lld ns\n", description, diff);
		break;
	case US:
		printf("%s: %lld.%03lld us\n", description, diff / 1000LL,
		       diff % 1000LL);
		break;
	case MS:
		printf("%s: %lld.%06lld ms\n", description, diff / 1000000LL,
		       diff % 1000000LL);
		break;
	case S:
		printf("%s: %lld.%09lld s\n", description, LmNsToS(diff),
		       diff % 1000000000LL);
		break;
	default:
		printf("Unknown format\n");
		break;
	}
}

// NOTE: (isa): Claude filled out the switch statement
void lm_log_timing(long long start, long long end, const char *description,
		   enum time_stamp_fmt stamp_fmt, enum lm_log_level log_level,
		   lm_log_module *log_module)
{
	long long diff = end - start;
	switch (stamp_fmt) {
	case S_NS:
		LmLogManual(log_level, log_module, "(%s): %lld.%09lld s",
			    description, LmNsToS(diff), diff % 1000000000LL);
		break;
	case S_US:
		LmLogManual(log_level, log_module, "%s: %lld.%06lld s",
			    description, LmNsToS(diff),
			    (diff % 1000000000LL) / 1000LL);
		break;
	case S_MS:
		LmLogManual(log_level, log_module, "%s: %lld.%03lld s",
			    description, LmNsToS(diff),
			    (diff % 1000000000LL) / 1000000LL);
		break;
	case US_TRUNK:
		LmLogManual(log_level, log_module, "%s: %lld us", description,
			    diff / 1000LL);
		break;
	case MS_TRUNK:
		LmLogManual(log_level, log_module, "%s: %lld ms", description,
			    diff / 1000000LL);
		break;
	case S_TRUNK:
		LmLogManual(log_level, log_module, "%s: %lld s", description,
			    diff / 1000000000LL);
		break;
	case NS:
		LmLogManual(log_level, log_module, "%s: %lld ns", description,
			    diff);
		break;
	case US:
		LmLogManual(log_level, log_module, "%s: %lld.%03lld us",
			    description, diff / 1000LL, diff % 1000LL);
		break;
	case MS:
		LmLogManual(log_level, log_module, "%s: %lld.%06lld ms",
			    description, diff / 1000000LL, diff % 1000000LL);
		break;
	case S:
		LmLogManual(log_level, log_module, "%s: %lld.%09lld s",
			    description, LmNsToS(diff), diff % 1000000000LL);
		break;
	default:
		LmLogManual(log_level, log_module, "Unknown format");
		break;
	}
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
		tc->percentage = ((double)(t2 - t1) / t2) * 100.0;
		tc->multiplier = (double)t2 / t1;
		return 1;
	} else {
		tc->percentage = ((double)(t1 - t2) / t2) * 100.0;
		tc->multiplier = (double)t1 / t2;
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

void lm_log_timing_comp(struct timing_comp tc, int res,
			enum lm_log_level log_level, lm_log_module *log_module)
{
	if (res == 0) {
		LmLogManual(
			log_level, log_module,
			"Both timestamps are identical (0%% difference, 1.00x)\n");
	} else if (res > 0) {
		LmLogManual(log_level, log_module,
			    "t1 is faster than t2 by %.2f%% (%.2fx faster)\n",
			    tc.percentage, tc.multiplier);
	} else {
		LmLogManual(log_level, log_module,
			    "t1 is slower than t2 by %.2f%% (%.2fx slower)\n",
			    tc.percentage, tc.multiplier);
	}
}
