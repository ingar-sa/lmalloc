#include <src/lm.h>
LM_LOG_REGISTER(timing);

#include <src/utils/system_info.h>

#include "timing.h"

#include <time.h>

// NOTE: (isa): Written by Claude
double calibrate_tsc(void)
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
		uint64_t start_tsc, end_tsc;
		double elapsed_sec, cycles_per_sec;

		clock_gettime(CLOCK_MONOTONIC, &start_ts);
		start_tsc = rdtsc();

		usleep(100000);

		end_tsc = rdtscp();
		clock_gettime(CLOCK_MONOTONIC, &end_ts);

		elapsed_sec = (double)(end_ts.tv_sec - start_ts.tv_sec) +
			      (double)(end_ts.tv_nsec - start_ts.tv_nsec) / 1e9;

		cycles_per_sec = (double)(end_tsc - start_tsc) / elapsed_sec;
		tsc_freq = cycles_per_sec;
		tsc_has_been_calibrated = true;
	}

	return tsc_freq;
}

long long lm_get_time_stamp(clockid_t type)
{
	struct timespec ts;
	(void)clock_gettime(type, &ts);
	// TODO: (isa): Consider checking return, but gotta go fast though
	long long stamp = LmSToNs(ts.tv_sec) + ts.tv_nsec;
	return stamp;
}

// NOTE: (isa): "Filled out" by Claude
static void format_timing(long long timing, enum time_stamp_fmt stamp_fmt,
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

#define TscToNs(tsc, freq) ((uint64_t)((double)(tsc) * 1e9 / (freq)))
#define TscToS(tsc, freq) ((uint64_t)((double)(tsc) / (freq)))

static void format_tsc_timing(uint64_t tsc_timing,
			      enum time_stamp_fmt stamp_fmt, char buf[64])
{
	double tsc_freq = calibrate_tsc();
	uint64_t timing_ns = TscToNs(tsc_timing, tsc_freq);

	switch (stamp_fmt) {
	case S_NS:
		snprintf(buf, 64, "%lu.%09lld s", TscToS(tsc_timing, tsc_freq),
			 timing_ns % 1000000000LL);
		break;
	case S_US:
		snprintf(buf, 64, "%lu.%06lld s", TscToS(tsc_timing, tsc_freq),
			 (timing_ns % 1000000000LL) / 1000LL);
		break;
	case S_MS:
		snprintf(buf, 64, "%lu.%03lld s", TscToS(tsc_timing, tsc_freq),
			 (timing_ns % 1000000000LL) / 1000000LL);
		break;
	case US_TRUNK:
		snprintf(buf, 64, "%lld us", timing_ns / 1000LL);
		break;
	case MS_TRUNK:
		snprintf(buf, 64, "%lld ms", timing_ns / 1000000LL);
		break;
	case S_TRUNK:
		snprintf(buf, 64, "%lld s", timing_ns / 1000000000LL);
		break;
	case NS:
		snprintf(buf, 64, "%lu ns", timing_ns);
		break;
	case US:
		snprintf(buf, 64, "%lld.%03lld us", timing_ns / 1000LL,
			 timing_ns % 1000LL);
		break;
	case MS:
		snprintf(buf, 64, "%lld.%06lld ms", timing_ns / 1000000LL,
			 timing_ns % 1000000LL);
		break;
	case S:
		snprintf(buf, 64, "%lu.%09lld s", TscToS(tsc_timing, tsc_freq),
			 timing_ns % 1000000000LL);
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
	format_timing(timing, stamp_fmt, timing_str);
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
	format_timing(timing, stamp_fmt, timing_str);
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
	format_timing(avg_ll, adjusted_fmt, timing_str);

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
	format_timing(avg_ll, adjusted_fmt, timing_str);

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

void lm_log_tsc_timing(uint64_t tsc, const char *description,
		       enum time_stamp_fmt stamp_fmt, bool log_raw,
		       enum lm_log_level log_level, lm_log_module *log_module)
{
	char timing_str[64];
	format_tsc_timing(tsc, stamp_fmt, timing_str);
	LmLogManual(log_module, log_raw, log_level, "%s: %s", description,
		    timing_str);
}

void lm_log_tsc_timing_avg(uint64_t tsc_timing, long long count,
			   const char *description,
			   enum time_stamp_fmt stamp_fmt, bool log_raw,
			   enum lm_log_level log_level,
			   lm_log_module *log_module)
{
	char timing_str[64];
	double tsc_freq = calibrate_tsc();

	uint64_t timing_ns = TscToNs(tsc_timing, tsc_freq);

	double avg = (double)timing_ns / (double)count;
	enum time_stamp_fmt adjusted_fmt = stamp_fmt;
	if (avg < 1000.0 && stamp_fmt != NS) {
		adjusted_fmt = NS;
	} else if (avg < 1000000.0 && stamp_fmt != NS && stamp_fmt != US) {
		adjusted_fmt = US;
	}
	long long avg_ll = (long long)avg;

	format_tsc_timing((uint64_t)(avg * (tsc_freq / 1e9)), adjusted_fmt,
			  timing_str);

	LmLogManual(log_module, log_raw, log_level,
		    "%s%s (total: %lu cycles ≈ %lu ns, count: %lld)",
		    description, timing_str, tsc_timing, timing_ns, count);
}

void lm_print_tsc_timing_avg(uint64_t tsc_timing, uint64_t count,
			     const char *description,
			     enum time_stamp_fmt stamp_fmt)
{
	char timing_str[64];
	double tsc_freq = calibrate_tsc();

	uint64_t timing_ns = TscToNs(tsc_timing, tsc_freq);

	double avg = (double)timing_ns / (double)count;
	enum time_stamp_fmt adjusted_fmt = stamp_fmt;
	if (avg < 1000.0 && stamp_fmt != NS) {
		adjusted_fmt = NS;
	} else if (avg < 1000000.0 && stamp_fmt != NS && stamp_fmt != US) {
		adjusted_fmt = US;
	}

	format_tsc_timing((uint64_t)(avg * (tsc_freq / 1e9)), adjusted_fmt,
			  timing_str);

	printf("%s%s (total: %lu cycles ≈ %lu ns, count: %lu)", description,
	       timing_str, tsc_timing, timing_ns, count);
}

int lm_compare_tsc_timing(uint64_t t1, uint64_t t2, struct timing_comp *tc)
{
	if (t1 == t2) {
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

void lm_print_tsc_timing_comp(struct timing_comp tc, int res)
{
	if (res == 0) {
		printf("Both TSC timings are identical (0%% difference, 1.00x)\n");
	} else if (res > 0) {
		printf("t1 is faster than t2 by %.2f%% (%.2fx faster)\n",
		       tc.percentage, tc.multiplier);
	} else {
		printf("t1 is slower than t2 by %.2f%% (%.2fx slower)\n",
		       tc.percentage, tc.multiplier);
	}
}

void lm_log_tsc_timing_comp(struct timing_comp tc, int res, bool log_raw,
			    enum lm_log_level log_level,
			    lm_log_module *log_module)
{
	if (res == 0) {
		LmLogManual(
			log_module, log_raw, log_level,
			"Both TSC timings are identical (0%% difference, 1.00x)\n");
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
