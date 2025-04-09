#ifndef LM_H_INCLUDE
#define LM_H_INCLUDE

// NOLINTBEGIN(misc-definitions-in-headers)

// WARN: Do not remove any includes without first checking if they are used in the implementation
// part by uncommenting #define LM_H_IMPLEMENTATION (and remember to comment it out again
// afterwards,
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#if !defined(__cplusplus)
#include <stdbool.h>
#endif

////////////////////////////////////////
//              DEFINES               //
////////////////////////////////////////

// NOTE: (ingar) Prevents the formatter from indenting everything inside the "C" block
#if defined(__cplusplus)
#define LM_BEGIN_EXTERN_C extern "C" {
#define LM_END_EXTERN_C }
#else
#define LM_BEGIN_EXTERN_C
#define LM_END_EXTERN_C
#endif

#if defined(__cplusplus)
LM_BEGIN_EXTERN_C
#endif

typedef unsigned int uint;

#define LM_LIKELY(x) __builtin_expect(!!(x), 1)
#define LM_UNLIKELY(x) __builtin_expect(!!(x), 0)

#define LM_EXPAND(x) x
#define LM__STRINGIFY__(x) #x
#define LM_STRINGIFY(x) LM__STRINGIFY__(x)

#define LM__CONCAT2__(x, y) x##y
#define LM_CONCAT2(x, y) LM__CONCAT2__(x, y)

#define LM__CONCAT3__(x, y, z) x##y##z
#define LM_CONCAT3(x, y, z) LM__CONCAT3__(x, y, z)

#define LmKiloByte(Number) (Number * 1000ULL)
#define LmMegaByte(Number) (LmKiloByte(Number) * 1000ULL)
#define LmGigaByte(Number) (LmMegaByte(Number) * 1000ULL)
#define LmTeraByte(Number) (LmGigaByte(Number) * 1000ULL)

#define LmKibiByte(Number) (Number * 1024ULL)
#define LmMebiByte(Number) (LmKibiByte(Number) * 1024ULL)
#define LmGibiByte(Number) (LmMebiByte(Number) * 1024ULL)
#define LmTebiByte(Number) (LmGibiByte(Number) * 1024ULL)

#define LmArrayLen(Array) (sizeof(Array) / sizeof(Array[0]))

#define LmMax(a, b) ((a > b) ? a : b)
#define LmMin(a, b) ((a < b) ? a : b)

#define LmIsPowerOfTwo(n) (n != 0 && (n & (n - 1)) == 0)

#define LmIsAligned(val, alignment) ((val % alignment) == 0)

#define LmPaddingToAlign(value, alignment) \
	((alignment - (value % alignment)) % alignment)

#define LmBoolToString(bool) ((bool) ? "true" : "false")

#define LmPtrDiff(a, b) (ptrdiff_t)((uintptr_t)a - (uintptr_t)b)

size_t lm_mem_sz_from_string(const char *string);

////////////////////////////////////////
//              LOGGING               //
////////////////////////////////////////
/* Based on the logging frontend I wrote for oec */

#if !defined(LM_LOG_BUF_SIZE)
#define LM_LOG_BUF_SIZE 4096
#endif

static_assert(LM_LOG_BUF_SIZE >= 128,
	      "LM_LOG_BUF_SIZE must greater than or equal to 128!");

#define LM_LOG_LEVEL_NONE (0U)
#define LM_LOG_LEVEL_ERR (1U)
#define LM_LOG_LEVEL_WRN (2U)
#define LM_LOG_LEVEL_INF (3U)
#define LM_LOG_LEVEL_DBG (4U)

#define LM_LOG_RAW true
#define LM_LOG_FMT false // TODO: (isa): This name is kinda bad. Change

enum lm_log_level {
	ERR = LM_LOG_LEVEL_ERR,
	WRN = LM_LOG_LEVEL_WRN,
	INF = LM_LOG_LEVEL_INF,
	DBG = LM_LOG_LEVEL_DBG,
};

#if !defined(LM_LOG_LEVEL)
#define LM_LOG_LEVEL 3
#endif

#if LM_LOG_LEVEL >= 4 && LM_PRINTF_DEBUG_ENABLE == 1
#define LmPrintfDebug(...) printf("DBG: " __VA_ARGS__);
#else
#define LmPrintfDebug(...)
#endif

#if LM_LOG_LEVEL >= LM_LOG_LEVEL_WRN && LM_PRINTF_WARN_ENABLE == 1
#define LmPrintfWarn(...) printf("WRN: " __VA_ARGS__);
#else
#define LmPrintfWarn(...)
#endif

typedef struct lm__log_module__ {
	const char *name;
	pthread_mutex_t lock;
	bool write_to_term; // True by default
	bool write_to_file; // False by default
	FILE *file;
	size_t buf_size;
	char *buf;
} lm__log_module__;

typedef struct lm__log_module__ lm_log_module;
#define LM_LOG_MODULE_LOCAL lm__log_instance__
#define LM_LOG_MODULE_GLOBAL lm__log_instance_global__

int lm__write_log__(lm__log_module__ *module, bool log_raw,
		    const char *log_level, const char *fmt, ...)
	__attribute__((format(printf, 4, 5)));

int lm__set_log_file_by_name__(const char *filename, const char *mode,
			       lm__log_module__ *module);
#define LmSetLogFileByNameLocal(filename, mode) \
	lm__set_log_file_by_name__(filename, mode, LM_LOG_MODULE_LOCAL)
#define LmSetLogFileByNameGlobal(filename, mode) \
	lm__set_log_file_by_name__(filename, mode, LM_LOG_MODULE_GLOBAL)

void lm__set_log_file__(FILE *file, lm__log_module__ *module);
#define LmSetLogFileLocal(file) lm__set_log_file__(file, LM_LOG_MODULE_LOCAL)
#define LmSetLogFileGlobal(file) lm__set_log_file__(file, LM_LOG_MODULE_GLOBAL)

void lm__remove_log_file__(lm__log_module__ *module);
#define LmRemoveLogFileLocal() lm__remove_log_file__(LM_LOG_MODULE_LOCAL)
#define LmRemoveLogFileGlobal() lm__remove_log_file__(LM_LOG_MODULE_GLOBAL)

void lm__enable_log_to_term__(lm__log_module__ *module);
#define LmEnableLogToTermLocal() lm__enable_log_to_term__(LM_LOG_MODULE_LOCAL)
#define LmEnableLogToTermGlobal() lm__enable_log_to_term__(LM_LOG_MODULE_GLOBAL)

void lm__disable_log_to_term__(lm__log_module__ *module);
#define LmDisableLogToTermLocal() lm__disable_log_to_term__(LM_LOG_MODULE_LOCAL)
#define LmDisableLogToTermGlobal() \
	lm__disable_log_to_term__(LM_LOG_MODULE_GLOBAL)

#define LM__LOG_LEVEL_CHECK__(level) (LM_LOG_LEVEL >= LM_LOG_LEVEL_##level)

#define LM_LOGGING_NOT_USED() \
	static lm__log_module__ *lm__log_instance__ __attribute__((used)) = NULL

#define LM_LOG_REGISTER(module_name)                                           \
	static char LM_CONCAT3(lm__log_module_, module_name,                   \
			       buf__)[LM_LOG_BUF_SIZE];                        \
	lm__log_module__ LM_CONCAT3(lm__log_module_, module_name, __)          \
		__attribute__((used)) = {                                      \
			.name = LM_STRINGIFY(module_name),                     \
			.lock = PTHREAD_MUTEX_INITIALIZER,                     \
			.write_to_term = true,                                 \
			.write_to_file = false,                                \
			.file = NULL,                                          \
			.buf_size = LM_LOG_BUF_SIZE,                           \
			.buf = LM_CONCAT3(lm__log_module_, module_name, buf__) \
		};                                                             \
	static lm__log_module__ *lm__log_instance__ __attribute__((used)) =    \
		&LM_CONCAT3(lm__log_module_, module_name, __)

#define LM_LOG_DECLARE(name)                                           \
	extern lm__log_module__ LM_CONCAT3(lm__log_module_, name, __); \
	static lm__log_module__ *lm__log_instance__                    \
		__attribute__((used)) = &LM_CONCAT3(lm__log_module_, name, __)

#define LM_LOG_GLOBAL_REGISTER()                                  \
	static char lm__log_module_global_buf__[LM_LOG_BUF_SIZE]; \
	lm__log_module__ lm__log_module_global__ __attribute__((  \
		used)) = { .name = "global",                      \
			   .lock = PTHREAD_MUTEX_INITIALIZER,     \
			   .write_to_term = true,                 \
			   .write_to_file = false,                \
			   .file = NULL,                          \
			   .buf_size = LM_LOG_BUF_SIZE,           \
			   .buf = lm__log_module_global_buf__ };  \
	lm__log_module__ *lm__log_instance_global__               \
		__attribute__((used)) = &lm__log_module_global__

#define LM_LOG_GLOBAL_DECLARE() \
	extern lm__log_module__ *lm__log_instance_global__

#define LM__LOG__(log_raw, log_level, fmt, ...)                                \
	do {                                                                   \
		if (LM__LOG_LEVEL_CHECK__(log_level)) {                        \
			int log_ret = lm__write_log__(lm__log_instance__,      \
						      log_raw,                 \
						      LM_STRINGIFY(log_level), \
						      fmt, ##__VA_ARGS__);     \
			assert(log_ret >= 0);                                  \
		}                                                              \
	} while (0)

#define LM__LOG_GLOBAL__(log_raw, log_level, fmt, ...)                        \
	do {                                                                  \
		if (LM__LOG_LEVEL_CHECK__(log_level)) {                       \
			int log_ret = lm__write_log__(                        \
				lm__log_instance_global__, log_raw,           \
				LM_STRINGIFY(log_level), fmt, ##__VA_ARGS__); \
			assert(log_ret >= 0);                                 \
		}                                                             \
	} while (0)

#define LM__LOG_MANUAL__(module, log_raw, log_level, fmt, ...)             \
	do {                                                               \
		if (log_level >= LM_LOG_LEVEL) {                           \
			int log_ret;                                       \
			switch (log_level) {                               \
			case ERR:                                          \
				log_ret = lm__write_log__(module, log_raw, \
							  "ERR", fmt,      \
							  ##__VA_ARGS__);  \
				break;                                     \
			case WRN:                                          \
				log_ret = lm__write_log__(module, log_raw, \
							  "WRN", fmt,      \
							  ##__VA_ARGS__);  \
				break;                                     \
			case INF:                                          \
				log_ret = lm__write_log__(module, log_raw, \
							  "INF", fmt,      \
							  ##__VA_ARGS__);  \
				break;                                     \
			case DBG:                                          \
				log_ret = lm__write_log__(module, log_raw, \
							  "DBG", fmt,      \
							  ##__VA_ARGS__);  \
				break;                                     \
			}                                                  \
		}                                                          \
	} while (0)

#define LmLogDebug(...) LM__LOG__(false, DBG, ##__VA_ARGS__)
#define LmLogInfo(...) LM__LOG__(false, INF, ##__VA_ARGS__)
#define LmLogWarning(...) LM__LOG__(false, WRN, ##__VA_ARGS__)
#define LmLogError(...) LM__LOG__(false, ERR, ##__VA_ARGS__)

#define LmLogDebugG(...) LM__LOG_GLOBAL__(false, DBG, ##__VA_ARGS__)
#define LmLogInfoG(...) LM__LOG_GLOBAL__(false, INF, ##__VA_ARGS__)
#define LmLogWarningG(...) LM__LOG_GLOBAL__(false, WRN, ##__VA_ARGS__)
#define LmLogErrorG(...) LM__LOG_GLOBAL__(false, ERR, ##__VA_ARGS__)

#define LmLogDebugR(...) LM__LOG__(true, DBG, ##__VA_ARGS__)
#define LmLogInfoR(...) LM__LOG__(true, INF, ##__VA_ARGS__)
#define LmLogWarningR(...) LM__LOG__(true, WRN, ##__VA_ARGS__)
#define LmLogErrorR(...) LM__LOG__(true, ERR, ##__VA_ARGS__)

#define LmLogDebugGR(...) LM__LOG_GLOBAL__(true, DBG, ##__VA_ARGS__)
#define LmLogInfoGR(...) LM__LOG_GLOBAL__(true, INF, ##__VA_ARGS__)
#define LmLogWarningGR(...) LM__LOG_GLOBAL__(true, WRN, ##__VA_ARGS__)
#define LmLogErrorGR(...) LM__LOG_GLOBAL__(true, ERR, ##__VA_ARGS__)

#define LmLogManual(...) LM__LOG_MANUAL__(__VA_ARGS__)

#if LM_ASSERT == 1
#define LmAssert(condition, fmt, ...)                                          \
	do {                                                                   \
		if (!(condition)) {                                            \
			lm__write_log__(lm__log_instance__, LM_LOG_FMT, "ERR", \
					"ASSERT (%s,%d): : " fmt, __func__,    \
					__LINE__, ##__VA_ARGS__);              \
			assert(condition);                                     \
		}                                                              \
	} while (0)
#else
#define LmAssert(...)
#endif

////////////////////////////////////////
//              MEMORY                //
////////////////////////////////////////

void *lm_memset_s(void *s, int c, size_t n);

////////////////////////////////////////
//              STRINGS               //
////////////////////////////////////////

typedef char *LmString;
typedef struct {
	size_t len;
	size_t cap;
} LmStringHeader;

#define LM_STRING_HEADER(str) ((LmStringHeader *)(str) - 1)

size_t lm_string_len(LmString string);
size_t lm_string_cap(LmString string);
size_t lm_string_space_avail(LmString string);
size_t lm_string_alloc_sz(LmString string);

LmString lm_string_make_space(LmString string, size_t add_len);
LmString lm_string_make(const char *string);
void lm_string_free(LmString string);

LmString lm_string_dup(const LmString string);
void lm_string_clear(LmString string);
LmString lm_string_set(LmString string, const char *c_string);
void lm_string_backspace(LmString string, size_t n);

LmString lm_string_append(LmString string, LmString other);
LmString lm_string_append_c(LmString string, const char *other);
LmString lm_string_append_fmt(LmString string, const char *fmt, ...)
	__attribute__((format(printf, 2, 3)));

bool lm_strings_are_equal(LmString lhs, LmString rhs);

////////////////////////////////////////
//            MEM TRACE               //
////////////////////////////////////////

void *lm__malloc_trace__(size_t size, int line, const char *func,
			 lm__log_module__ *module);
void *lm__calloc_trace__(size_t ElementCount, size_t Elementsize, int line,
			 const char *func, lm__log_module__ *module);
void *lm__realloc_trace__(void *ptr, size_t size, int line, const char *func,
			  lm__log_module__ *module);
void lm__free_trace__(void *ptr, int line, const char *func,
		      lm__log_module__ *module);

#if LM_MEM_TRACE == 1
#define malloc(size) \
	lm__malloc_trace__(size, __LINE__, __func__, lm__log_instance__)
#define calloc(count, size) \
	lm__calloc_trace__(count, size, __LINE__, __func__, lm__log_instance__)
#define realloc(ptr, size) \
	lm__realloc_trace__(ptr, size, __LINE__, __func__, lm__log_instance__)
#define free(ptr) lm__free_trace__(ptr, __LINE__, __func__, lm__log_instance__)
#endif

/////////////////////////////////////////
//              FILE IO                //
/////////////////////////////////////////

typedef struct lm_file_data {
	size_t size;
	uint8_t *data;
} lm_file_data;

lm_file_data *lm_load_file_into_memory(const char *filename);
void lm_free_file_data(lm_file_data **file_data);

FILE *lm_open_file_by_name(const char *filename, const char *mode);
int lm_close_file(FILE *file);

int lm_write_bytes_to_file(const uint8_t *buf, size_t size, FILE *file);
int lm_write_bytes_to_file_by_name(const uint8_t *buf, size_t size,
				   const char *filename);

////////////////////////////////////////
//              SYSTEM                //
////////////////////////////////////////

size_t lm_get_l1_cache_line_size(void);

////////////////////////////////////////
//           END OF INCLUDE           //
////////////////////////////////////////

#if defined(__cplusplus)
LM_END_EXTERN_C
#endif
#endif // LM_H_INCLUDE

////////////////////////////////////////
//     BEGINNING OF IMPLEMENTATION    //
////////////////////////////////////////

// WARN: Only one file in a program should define LM_H_IMPLEMENTATION, otherwise you will get
// redefintion errors
//#define LM_H_IMPLEMENTATION // Uncomment/comment to enable/disable syntax highlighting
#ifdef LM_H_IMPLEMENTATION

LM_LOG_GLOBAL_DECLARE(); // Enables use of logging functionality inside this header file
#if LM_MEM_TRACE == 1
#undef malloc
#undef calloc
#undef realloc
#undef free
#endif

////////////////////////////////////////
//               MISC                 //
////////////////////////////////////////

// NOTE: (isa): Written by Claude
size_t lm_mem_sz_from_string(const char *string)
{
	if (!string || !*string) {
		return 0;
	}

	char *end_ptr;
	size_t value = strtoull(string, &end_ptr, 10);

	if (end_ptr == string) {
		return 0; // No number found
	}

	while (isspace(*end_ptr)) {
		end_ptr++;
	}

	if (!*end_ptr) {
		return value;
	}

	size_t multiplier = 0;
	switch (*end_ptr) {
	case 'T':
		multiplier = 1000ULL * 1000ULL * 1000ULL * 1000ULL;
		break;
	case 't':
		multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
		break;
	case 'G':
		multiplier = 1000ULL * 1000ULL * 1000ULL;
		break;
	case 'g':
		multiplier = 1024ULL * 1024ULL * 1024ULL;
		break;
	case 'M':
		multiplier = 1000ULL * 1000ULL;
		break;
	case 'm':
		multiplier = 1024ULL * 1024ULL;
		break;
	case 'K':
		multiplier = 1000ULL;
		break;
	case 'k':
		multiplier = 1024ULL;
		break;
	case 'B':
		// Check if this is just 'B' or part of a larger unit
		if (end_ptr[1] != '\0') {
			return 0;
		} else {
			multiplier = 1;
		}
		break;
	default:
		return 0;
	}

	// If we have a unit letter, verify it's followed by 'B' (except for plain 'B')
	if (*end_ptr != 'B' && (end_ptr[1] != 'B' || end_ptr[2] != '\0')) {
		return 0;
	}

	return value * multiplier;
}
////////////////////////////////////////
//              LOGGING               //
////////////////////////////////////////
/* Based on the logging frontend I wrote for oec */

int lm__write_log__(lm__log_module__ *module, bool log_raw,
		    const char *log_level, const char *fmt, ...)
{
	if (module == NULL) {
		return 0; // NOTE(ingar): If the log module is NULL, then we assume logging isn't used
	}

	pthread_mutex_lock(&module->lock);

	int retval = 0;

	size_t chars_written = 0;
	size_t buf_remaining = module->buf_size;
	int fmt_ret = 0;

	if (!log_raw) {
		time_t posix_time;
		struct tm time_info;

		if ((time_t)(-1) == time(&posix_time)) {
			retval = -errno;
			goto out;
		}

		if (NULL == localtime_r(&posix_time, &time_info)) {
			retval = -errno;
			goto out;
		}

		int fmt_ret = strftime(module->buf, buf_remaining,
				       "%T::", &time_info);
		if (0 == fmt_ret) {
			// NOTE(ingar): Since the buffer size is at least 128, this should never happen
			assert(fmt_ret);
			retval = -ENOMEM;
			goto out;
		}

		chars_written = fmt_ret;
		buf_remaining -= fmt_ret;

		fmt_ret = snprintf(module->buf + chars_written, buf_remaining,
				   "%s::%s: ", module->name, log_level);
		if (fmt_ret < 0) {
			retval = -errno;
			goto out;
		} else if ((size_t)fmt_ret >= buf_remaining) {
			// NOTE(ingar): If the log module name is so long that it does not fit in 128 bytes - the
			// time stamp, it should be changed
			assert(fmt_ret);
			retval = -ENOMEM;
			goto out;
		}

		chars_written += fmt_ret;
		buf_remaining -= fmt_ret;
	}

	va_list va_args;
	va_start(va_args, fmt);
	fmt_ret = vsnprintf(module->buf + chars_written, buf_remaining, fmt,
			    va_args);
	va_end(va_args);

	if (fmt_ret < 0) {
		retval = -errno;
		goto out;
	} else if ((size_t)fmt_ret >= buf_remaining) {
		(void)memset(module->buf + chars_written, 0, buf_remaining);
		fmt_ret = snprintf(module->buf + chars_written, buf_remaining,
				   "%s", "Message dropped; too big");
		if (fmt_ret < 0) {
			retval = -errno;
			goto out;
		} else if ((size_t)fmt_ret >= buf_remaining) {
			assert(fmt_ret);
			retval = -ENOMEM;
			goto out;
		}
	}

	chars_written += fmt_ret;
	module->buf[chars_written++] = '\n';

	if (module->write_to_term) {
		int out_fd;
		if ('E' == log_level[0]) {
			out_fd = STDERR_FILENO;
		} else {
			out_fd = STDOUT_FILENO;
		}

		if ((ssize_t)(-1) ==
		    write(out_fd, module->buf, chars_written)) {
			retval = -errno;
			goto out;
		}
	}

	if (module->write_to_file) {
		if ((size_t)1 !=
		    fwrite(module->buf, chars_written, 1, module->file)) {
			retval = -ferror(module->file);
			goto out;
		}
	}

out:
	pthread_mutex_unlock(&module->lock);
	return retval;
}

void lm__set_log_file__(FILE *file, lm__log_module__ *module)
{
	pthread_mutex_lock(&module->lock);
	module->write_to_file = true;
	module->file = file;
	pthread_mutex_unlock(&module->lock);
}

int lm__set_log_file_by_name__(const char *filename, const char *mode,
			       lm__log_module__ *module)
{
	FILE *file = fopen(filename, mode);
	if (!file) {
		LmLogErrorG("Failed to open log file %s in mode %s: %s",
			    filename, mode, strerror(errno));
		return -1;
	}

	lm__set_log_file__(file, module);
	return 0;
}

void lm__remove_log_file__(lm__log_module__ *module)
{
	pthread_mutex_lock(&module->lock);
	module->write_to_file = false;
	module->file = NULL;
	pthread_mutex_unlock(&module->lock);
}

void lm__enable_log_to_term__(lm__log_module__ *module)
{
	pthread_mutex_lock(&module->lock);
	module->write_to_term = true;
	pthread_mutex_unlock(&module->lock);
}

void lm__disable_log_to_term__(lm__log_module__ *module)
{
	pthread_mutex_lock(&module->lock);
	module->write_to_term = false;
	pthread_mutex_lock(&module->lock);
}

////////////////////////////////////////
//              MEMORY                //
////////////////////////////////////////

/* From https://github.com/BLAKE2/BLAKE2/blob/master/ref/blake2-impl.h */
void *lm_memset_s(void *dest, int ch, size_t count)
{
	// NOTE(ingar): This prevents the compiler from optimizing away memset. This could be replaced
	// by a call to memset_s if one uses C11, but this method should be compatible with earlier C
	// standards
	static void *(*const volatile memset_v)(void *, int, size_t) = &memset;
	return memset_v(dest, ch, count);
}

////////////////////////////////////////
//              STRINGS               //
////////////////////////////////////////

size_t lm_string_len(LmString string)
{
	return LM_STRING_HEADER(string)->len;
}
size_t lm_string_cap(LmString string);

size_t lm_string_cap(LmString string)
{
	return LM_STRING_HEADER(string)->cap;
}

size_t lm_string_space_avail(LmString string)
{
	LmStringHeader *header = LM_STRING_HEADER(string);
	return header->cap - header->len;
}

size_t lm_string_alloc_sz(LmString string)
{
	size_t sz = sizeof(LmStringHeader) + lm_string_cap(string);
	return sz;
}

void lm__string_set_len__(LmString string, size_t len)
{
	LM_STRING_HEADER(string)->len = len;
}

void lm__string_set_cap__(LmString string, size_t cap)
{
	LM_STRING_HEADER(string)->cap = cap;
}

LmString lm__string_make__(const void *init_string, size_t len)
{
	size_t header_sz = sizeof(LmStringHeader);
	void *ptr = calloc(1, header_sz + len + 1);
	if (ptr == NULL) {
		return NULL;
	}
	if (init_string == NULL) {
		memset(ptr, 0, header_sz + len + 1);
	}

	LmString string;
	LmStringHeader *header;

	string = (char *)ptr + header_sz;
	header = LM_STRING_HEADER(string);

	header->len = len;
	header->cap = len;
	if (len > 0 && (init_string != NULL)) {
		memcpy(string, init_string, len);
		string[len] = '\0';
	}

	return string;
}

LmString lm_string_make(const char *string)
{
	size_t Len = (string != NULL) ? strlen(string) : 0;
	return lm__string_make__(string, Len);
}

void lm_string_free(LmString string)
{
	if (string != NULL) {
		LmStringHeader *h = LM_STRING_HEADER(string);
		free(h);
	}
}

LmString lm_string_dup(const LmString string)
{
	return lm__string_make__(string, lm_string_len(string));
}

void lm_string_clear(LmString string)
{
	lm__string_set_len__(string, 0);
	string[0] = '\0';
}

void lm_string_backspace(LmString string, size_t n)
{
	size_t new_len = lm_string_len(string) - n;
	lm__string_set_len__(string, new_len);
	string[new_len] = '\0';
}

LmString lm_string_make_space(LmString string, size_t add_len)
{
	size_t available = lm_string_space_avail(string);
	if (available < add_len) {
		LmStringHeader *header = LM_STRING_HEADER(string);
		size_t new_sz = sizeof(LmStringHeader) + header->cap + add_len;

		LmStringHeader *new_string = realloc(header, new_sz);
		if (new_string == NULL)
			return NULL;

		size_t new_cap = new_string->cap + add_len;
		string =
			(LmString)((char *)new_string + sizeof(LmStringHeader));
		lm__string_set_cap__(string, new_cap);
	}
	return string;
}

LmString lm__string_append__(LmString string, const void *other,
			     size_t other_len)
{
	string = lm_string_make_space(string, other_len);
	if (string == NULL)
		return NULL;

	size_t cur_len = lm_string_len(string);
	memcpy(string + cur_len, other, other_len);
	lm__string_set_len__(string, cur_len + other_len);
	string[cur_len + other_len] = '\0';
	return string;
}

LmString lm_string_append(LmString String, LmString Other)
{
	return lm__string_append__(String, Other, lm_string_len(Other));
}

LmString lm_string_append_c(LmString string, const char *other)
{
	return lm__string_append__(string, other, strlen(other));
}

LmString lm_string_append_fmt(LmString string, const char *fmt, ...)
{
	char buf[1024] = { 0 };
	va_list va;
	va_start(va, fmt);
	size_t fmt_len = vsnprintf(buf, LmArrayLen(buf) - 1, fmt, va);
	va_end(va);
	return lm__string_append__(string, buf, fmt_len);
}

LmString lm_string_set(LmString string, const char *c_string)
{
	size_t len = strlen(c_string);
	string = lm_string_make_space(string, len);
	if (string == NULL) {
		return NULL;
	}

	memcpy(string, c_string, len);
	string[len] = '\0';
	lm__string_set_len__(string, len);

	return string;
}

bool lm_strings_are_equal(LmString lhs, LmString rhs)
{
	size_t l_string_len = lm_string_len(lhs);
	size_t r_string_len = lm_string_len(rhs);

	if (l_string_len != r_string_len) {
		return false;
	} else {
		return memcmp(lhs, rhs, l_string_len);
	}
}
////////////////////////////////////////
//            MEM TRACE               //
////////////////////////////////////////

void *lm__malloc_trace__(size_t size, int line, const char *func,
			 lm__log_module__ *module)
{
	void *ptr = malloc(size);
	lm__write_log__(module, "DBG", "MALLOC (%s,%d): %p (%zd B)", func, line,
			ptr, size);
	return ptr;
}

void *lm__calloc_trace__(size_t count, size_t size, int line, const char *func,
			 lm__log_module__ *module)
{
	void *ptr = calloc(count, size);
	lm__write_log__(module, "DBG",
			"CALLOC (%s,%d): %p (%lu * %luB = %zd B)", func, line,
			ptr, count, size, (count * size));
	return ptr;
}

void *lm__realloc_trace__(void *ptr, size_t size, int line, const char *func,
			  lm__log_module__ *module)
{
	void *original = ptr;
	void *reallocd = NULL;
	if (ptr != NULL) {
		reallocd = realloc(ptr, size);
	}
	lm__write_log__(module, "DBG", "REALLOC (%s,%d): %p -> %p (%zd B)",
			func, line, original, reallocd, size);
	return reallocd;
}

void lm__free_trace__(void *ptr, int line, const char *func,
		      lm__log_module__ *module)
{
	lm__write_log__(module, "DBG", "FREE (%s,%d): %p", func, line, ptr);
	if (ptr != NULL) {
		free(ptr);
	}
}

lm_file_data *lm_load_file_into_memory(const char *filename)
{
	FILE *file = fopen(filename, "rb");
	if (!file) {
		LmLogErrorG("Failed to open file: %s", strerror(errno));
		return NULL;
	}

	if (fseek(file, 0L, SEEK_END) != 0) {
		LmLogErrorG("Failed to seek in file %s", filename);
		fclose(file);
		return NULL;
	}

	size_t file_size = (size_t)ftell(file);
	rewind(file);

	lm_file_data *file_data;
	size_t file_data_size = sizeof(lm_file_data) + file_size + 1;
	file_data = calloc(1, file_data_size);
	if (!file_data) {
		LmLogErrorG(
			"Failed to allocate memory for file data for file %s",
			filename);
		fclose(file);
		return NULL;
	}

	file_data->size = file_size;
	file_data->data = (uint8_t *)file_data + sizeof(lm_file_data);

	if (fread(file_data->data, file_size, 1, file) != 1) {
		LmLogErrorG("Failed to read data from file %s: %s", filename,
			    strerror(errno));
		fclose(file);
		free(file_data);
		return NULL;
	}

	fclose(file);
	return file_data;
}

void lm_free_file_data(lm_file_data **file_data)
{
	if (file_data && *file_data) {
		free(*file_data);
		*file_data = NULL;
	} else {
		LmLogWarningG(
			" Pointer to file data pointer or file data pointer was NULL");
	}
}

FILE *lm_open_file_by_name(const char *filename, const char *mode)
{
	FILE *file = fopen(filename, mode);
	if (!file) {
		LmLogErrorG("Failed to open file %s in mode %s: %s", filename,
			    mode, strerror(errno));
		return NULL;
	}

	return file;
}

int lm_close_file(FILE *file)
{
	if (EOF == fclose(file)) {
		LmLogErrorG("Failed to close file %p", (void *)file);
		return -EOF;
	}

	return 0;
}

int lm_write_bytes_to_file(const uint8_t *buf, size_t size, FILE *file)
{
	if (!(fwrite(buf, size, 1, file) == (size_t)1)) {
		LmLogErrorG("Failed to write buffer to file %p: %s",
			    (void *)file, strerror(errno));
		return -errno;
	}

	return 0;
}

int lm_write_bytes_to_file_by_name(const uint8_t *buf, size_t size,
				   const char *filename)
{
	FILE *file = fopen(filename, "wb");
	if (!file) {
		LmLogErrorG("Unable to open file %s", filename);
		return errno;
	}

	if (!(fwrite(buf, size, 1, file) == (size_t)1)) {
		LmLogErrorG("Failed to write buffer to file %s: %s", filename,
			    strerror(errno));
		return errno;
	}

	fclose(file);
	return 0;
}

// NOLINTEND(misc-definitions-in-headers)

#endif // LM_H_IMPLEMENTATION
