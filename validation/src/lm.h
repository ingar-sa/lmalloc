#ifndef LM_H_INCLUDE
#define LM_H_INCLUDE

// NOLINTBEGIN(misc-definitions-in-headers)

// WARN: Do not remove any includes without first checking if they are used in the implementation
// part by uncommenting #define LM_H_IMPLEMENTATION (and remember to comment it out again
// afterwards,
#include <assert.h>
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
	size_t buf_size;
	char *buf;
	pthread_mutex_t lock;
} lm__log_module__;

typedef struct lm__log_module__ lm_log_module;
#define LM_LOG_MODULE_LOCAL lm__log_instance__
#define LM_LOG_MODULE_GLOBAL lm__log_instance_global__

int lm__write_log__(lm__log_module__ *module, const char *log_level,
		    const char *fmt, ...) __attribute__((format(printf, 3, 4)));

#define LM__LOG_LEVEL_CHECK__(level) (LM_LOG_LEVEL >= LM_LOG_LEVEL_##level)

#define LM_LOGGING_NOT_USED() \
	static lm__log_module__ *lm__log_instance__ __attribute__((used)) = NULL

#define LM_LOG_REGISTER(module_name)                                        \
	static char LM_CONCAT3(lm__log_module_, module_name,                \
			       buf__)[LM_LOG_BUF_SIZE];                     \
	lm__log_module__ LM_CONCAT3(lm__log_module_, module_name,           \
				    __) __attribute__((used)) = {           \
		.name = LM_STRINGIFY(module_name),                          \
		.buf_size = LM_LOG_BUF_SIZE,                                \
		.buf = LM_CONCAT3(lm__log_module_, module_name, buf__),     \
		.lock = PTHREAD_MUTEX_INITIALIZER                           \
	};                                                                  \
	static lm__log_module__ *lm__log_instance__ __attribute__((used)) = \
		&LM_CONCAT3(lm__log_module_, module_name, __)

#define LM_LOG_DECLARE(name)                                           \
	extern lm__log_module__ LM_CONCAT3(lm__log_module_, name, __); \
	static lm__log_module__ *lm__log_instance__                    \
		__attribute__((used)) = &LM_CONCAT3(lm__log_module_, name, __)

#define LM_LOG_GLOBAL_REGISTER()                                               \
	static char lm__log_module_global_buf__[LM_LOG_BUF_SIZE];              \
	lm__log_module__ lm__log_module_global__                               \
		__attribute__((used)) = { .name = "global",                    \
					  .buf_size = LM_LOG_BUF_SIZE,         \
					  .buf = lm__log_module_global_buf__,  \
					  .lock = PTHREAD_MUTEX_INITIALIZER }; \
	lm__log_module__ *lm__log_instance_global__                            \
		__attribute__((used)) = &lm__log_module_global__

#define LM_LOG_GLOBAL_DECLARE() \
	extern lm__log_module__ *lm__log_instance_global__

#define LM__LOG__(log_level, fmt, ...)                                         \
	do {                                                                   \
		if (LM__LOG_LEVEL_CHECK__(log_level)) {                        \
			int log_ret = lm__write_log__(lm__log_instance__,      \
						      LM_STRINGIFY(log_level), \
						      fmt, ##__VA_ARGS__);     \
			assert(log_ret >= 0);                                  \
		}                                                              \
	} while (0)

#define LM__LOG_GLOBAL__(log_level, fmt, ...)                                 \
	do {                                                                  \
		if (LM__LOG_LEVEL_CHECK__(log_level)) {                       \
			int log_ret = lm__write_log__(                        \
				lm__log_instance_global__,                    \
				LM_STRINGIFY(log_level), fmt, ##__VA_ARGS__); \
			assert(log_ret >= 0);                                 \
		}                                                             \
	} while (0)

#define LM__LOG_MANUAL__(log_level, module, fmt, ...)                         \
	do {                                                                  \
		if (log_level >= LM_LOG_LEVEL) {                              \
			int log_ret;                                          \
			switch (log_level) {                                  \
			case ERR:                                             \
				log_ret = lm__write_log__(module, "ERR", fmt, \
							  ##__VA_ARGS__);     \
				break;                                        \
			case WRN:                                             \
				log_ret = lm__write_log__(module, "WRN", fmt, \
							  ##__VA_ARGS__);     \
				break;                                        \
			case INF:                                             \
				log_ret = lm__write_log__(module, "INF", fmt, \
							  ##__VA_ARGS__);     \
				break;                                        \
			case DBG:                                             \
				log_ret = lm__write_log__(module, "DBG", fmt, \
							  ##__VA_ARGS__);     \
				break;                                        \
			}                                                     \
		}                                                             \
	} while (0)

#define LmLogDebug(...) LM__LOG__(DBG, ##__VA_ARGS__)
#define LmLogInfo(...) LM__LOG__(INF, ##__VA_ARGS__)
#define LmLogWarning(...) LM__LOG__(WRN, ##__VA_ARGS__)
#define LmLogError(...) LM__LOG__(ERR, ##__VA_ARGS__)

#define LmLogDebugG(...) LM__LOG_GLOBAL__(DBG, ##__VA_ARGS__)
#define LmLogInfoG(...) LM__LOG_GLOBAL__(INF, ##__VA_ARGS__)
#define LmLogWarningG(...) LM__LOG_GLOBAL__(WRN, ##__VA_ARGS__)
#define LmLogErrorG(...) LM__LOG_GLOBAL__(ERR, ##__VA_ARGS__)

#define LmLogManual(...) LM__LOG_MANUAL__(__VA_ARGS__)

#if LM_ASSERT == 1
#define LmAssert(condition, fmt, ...)                                       \
	do {                                                                \
		if (!(condition)) {                                         \
			lm__write_log__(lm__log_instance__, "DBG",          \
					"ASSERT (%s,%d): : " fmt, __func__, \
					__LINE__, ##__VA_ARGS__);           \
			assert(condition);                                  \
		}                                                           \
	} while (0)
#else
#define LmAssert(...)
#endif

////////////////////////////////////////
//              MEMORY                //
////////////////////////////////////////

void *lm_memset_s(void *s, int c, size_t n);

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

lm_file_data *lm_load_file_into_mem(const char *filename);
void lm_free_file_data(lm_file_data **file_data);
int lm_write_bytes_to_file(const uint8_t *buf, size_t size,
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

// WARN: Only one file in a program should define SDB_H_IMPLEMENTATION, otherwise you will get
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
//              LOGGING               //
////////////////////////////////////////
/* Based on the logging frontend I wrote for oec */

int lm__write_log__(lm__log_module__ *module, const char *log_level,
		    const char *fmt, ...)
{
	if (module == NULL) {
		return 0; // NOTE(ingar): If the log module is NULL, then we assume logging isn't used
	}

	pthread_mutex_lock(&module->lock);

	time_t posix_time;
	struct tm time_info;

	if ((time_t)(-1) == time(&posix_time)) {
		return -errno;
	}

	if (NULL == localtime_r(&posix_time, &time_info)) {
		return -errno;
	}

	size_t chars_written = 0;
	size_t buf_remaining = module->buf_size;

	int fmt_ret = strftime(module->buf, buf_remaining, "%T: ", &time_info);
	if (0 == fmt_ret) {
		// NOTE(ingar): Since the buffer size is at least 128, this should never happen
		assert(fmt_ret);
		return -ENOMEM;
	}

	chars_written = fmt_ret;
	buf_remaining -= fmt_ret;

	fmt_ret = snprintf(module->buf + chars_written, buf_remaining,
			   "%s: %s: ", module->name, log_level);
	if (fmt_ret < 0) {
		return -errno;
	} else if ((size_t)fmt_ret >= buf_remaining) {
		// NOTE(ingar): If the log module name is so long that it does not fit in 128 bytes - the
		// time stamp, it should be changed
		assert(fmt_ret);
		return -ENOMEM;
	}

	chars_written += fmt_ret;
	buf_remaining -= fmt_ret;

	va_list va_args;
	va_start(va_args, fmt);
	fmt_ret = vsnprintf(module->buf + chars_written, buf_remaining, fmt,
			    va_args);
	va_end(va_args);

	if (fmt_ret < 0) {
		return -errno;
	} else if ((size_t)fmt_ret >= buf_remaining) {
		(void)memset(module->buf + chars_written, 0, buf_remaining);
		fmt_ret = snprintf(module->buf + chars_written, buf_remaining,
				   "%s", "Message dropped; too big");
		if (fmt_ret < 0) {
			return -errno;
		} else if ((size_t)fmt_ret >= buf_remaining) {
			assert(fmt_ret);
			return -ENOMEM;
		}
	}

	chars_written += fmt_ret;
	module->buf[chars_written++] = '\n';

	int out_fd;
	if ('E' == log_level[0]) {
		out_fd = STDERR_FILENO;
	} else {
		out_fd = STDOUT_FILENO;
	}

	if ((ssize_t)(-1) == write(out_fd, module->buf, chars_written)) {
		return -errno;
	}

	pthread_mutex_unlock(&module->lock);

	return 0;
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

lm_file_data *lm_load_file_into_mem(const char *filename)
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

	size_t bytes_read = fread(file_data->data, 1, file_size, file);
	if (bytes_read != file_size) {
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

int lm_write_bytes_to_file(const uint8_t *buf, size_t size,
			   const char *filename)
{
	FILE *file = fopen(filename, "wb");
	if (!file) {
		LmLogErrorG("Unable to open file %s", filename);
		return errno;
	}

	if (!(fwrite(buf, size, 1, file) == 1)) {
		LmLogErrorG("Failed to write buffer to file %s: %s", filename,
			    strerror(errno));
		return errno;
	}

	fclose(file);
	return 0;
}

////////////////////////////////////////
//              SYSTEM                //
////////////////////////////////////////

// NOTE: Original written by claude
size_t lm_get_l1_cache_line_size(void)
{
	FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
	if (cpuinfo == NULL) {
		LmLogErrorG("Failed to open /proc/cpuinfo");
		return 0;
	}

	char line[256];
	size_t line_size = 0;

	// First try to find cache_alignment in /proc/cpuinfo
	while (fgets(line, sizeof(line), cpuinfo)) {
		if (strstr(line, "cache_alignment") != NULL) {
			char *value = strchr(line, ':');
			if (value) {
				line_size =
					(size_t)strtoul(value + 1, NULL, 10);
				break;
			}
		}
	}
	fclose(cpuinfo);

	// If not found in /proc/cpuinfo, try a fallback method
	if (line_size == 0) {
		LmLogDebugG(
			"Could not determine L1 cache line size from /proc/cpuinfo. Trying sysconf");

		// Use sysconf or try to read from sysfs
		FILE *cache_line = fopen(
			"/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size",
			"r");
		if (cache_line) {
			if (fscanf(cache_line, "%zu", &line_size) != 1) {
				line_size = 0;
			}
			fclose(cache_line);
		}
		if (line_size == 0) {
			LmLogDebugG(
				"Could not determine L1 cache line size from sysconf. Trying asm");
		} else {
			LmLogDebugG("L1 cache line size (from sysconf) is %zd",
				    line_size);
		}
	} else {
		LmLogDebugG("L1 cache line size (from /proc/cpuinfo) is %zd",
			    line_size);
	}

	// Third fallback - most x86 processors have 64-byte cache lines
	if (line_size == 0) {
		// Try using a direct x86 CPUID instruction
#if defined(__x86_64__) || defined(__i386__)
		unsigned int eax, ebx, ecx, edx;

		// Try CPUID leaf 1 first (older method)
		__asm__ __volatile__("cpuid"
				     : "=a"(eax), "=b"(ebx), "=c"(ecx),
				       "=d"(edx)
				     : "a"(1));

		// CLFLUSH line size is in bits 15-8 of EBX, in units of 8 bytes
		line_size = ((ebx >> 8) & 0xff) * 8;

		// If that didn't work, try CPUID leaf 4 (newer method)
		if (line_size == 0) {
			__asm__ __volatile__("cpuid"
					     : "=a"(eax), "=b"(ebx), "=c"(ecx),
					       "=d"(edx)
					     : "a"(4), "c"(0));

			if ((eax & 0x1f) != 0) {
				// Cache line size is in bits 0-11 of EBX, plus 1
				line_size = (ebx & 0xfff) + 1;
			}
		}
#endif

		// Last resort - assume a common default
		if (line_size == 0) {
			LmLogWarningG(
				"Could not determine cache line size. Using default of 64");
			line_size =
				64; // Most common L1 cache line size on modern CPUs

		} else {
			LmLogDebugG("L1 cache line size (from asm) is %zd",
				    line_size);
		}
	}

	return line_size;
}

// NOLINTEND(misc-definitions-in-headers)

#endif // LM_H_IMPLEMENTATION
