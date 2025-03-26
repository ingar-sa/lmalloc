#ifndef EIT_H_INCLUDE
#define EIT_H_INCLUDE

// NOLINTBEGIN(misc-definitions-in-headers)

// WARN: Do not remove any includes without first checking if they are used in the implementation
// part by uncommenting #define EIT_H_IMPLEMENTATION (and remember to comment it out again
// afterwards,
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>
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

// NOTE(ingar): Prevents the formatter from indenting everything inside the "C" block
#if defined(__cplusplus)
#define EIT_BEGIN_EXTERN_C extern "C" {
#define EIT_END_EXTERN_C }
#else
#define EIT_BEGIN_EXTERN_C
#define EIT_END_EXTERN_C
#endif

#if defined(__cplusplus)
EIT_BEGIN_EXTERN_C
#endif

#define EIT_EXPAND(x) x
#define EIT__STRINGIFY__(x) #x
#define EIT_STRINGIFY(x) EIT__STRINGIFY__(x)

#define EIT__CONCAT2__(x, y) x##y
#define EIT_CONCAT2(x, y) EIT__CONCAT2__(x, y)

#define EIT__CONCAT3__(x, y, z) x##y##z
#define EIT_CONCAT3(x, y, z) EIT__CONCAT3__(x, y, z)

#define EitKiloByte(Number) (Number * 1000ULL)
#define EitMegaByte(Number) (EitKiloByte(Number) * 1000ULL)
#define EitGigaByte(Number) (EitMegaByte(Number) * 1000ULL)
#define EitTeraByte(Number) (EitGigaByte(Number) * 1000ULL)

#define EitKibiByte(Number) (Number * 1024ULL)
#define EitMebiByte(Number) (EitKibiByte(Number) * 1024ULL)
#define EitGibiByte(Number) (EitMebiByte(Number) * 1024ULL)
#define EitTebiByte(Number) (EitGibiByte(Number) * 1024ULL)

#define EitArrayLen(Array) (sizeof(Array) / sizeof(Array[0]))

#define EitMax(a, b) ((a > b) ? a : b)
#define EitMin(a, b) ((a < b) ? a : b)

#define EitIsPowerOfTwo(n) (n != 0 && (n & (n - 1)) == 0)

#define EitIsAligned(val, alignment) ((val % alignment) == 0)

#define EitPaddingToAlign(value, alignment) \
	((alignment - (value % alignment)) % alignment)

#define EitBoolToString(bool) ((bool) ? "true" : "false")

#define EitPtrDiff(a, b) (ptrdiff_t)((uintptr_t)a - (uintptr_t)b)

////////////////////////////////////////
//              LOGGING               //
////////////////////////////////////////
/* Based on the logging frontend I wrote for oec */

#if !defined(EIT_LOG_BUF_SIZE)
#define EIT_LOG_BUF_SIZE 4096
#endif

static_assert(EIT_LOG_BUF_SIZE >= 128,
	      "EIT_LOG_BUF_SIZE must greater than or equal to 128!");

#define EIT_LOG_LEVEL_NONE (0U)
#define EIT_LOG_LEVEL_ERR (1U)
#define EIT_LOG_LEVEL_WRN (2U)
#define EIT_LOG_LEVEL_INF (3U)
#define EIT_LOG_LEVEL_DBG (4U)

#if !defined(EIT_LOG_LEVEL)
#define EIT_LOG_LEVEL 3
#endif

#if EIT_LOG_LEVEL >= 4 && EIT_PRINTF_DEBUG_ENABLE == 1
#define SdbPrintfDebug(...) printf(__VA_ARGS__);
#else
#define SdbPrintfDebug(...)
#endif

typedef struct sdb__log_module__ {
	const char *name;
	size_t buf_size;
	char *buf;
	pthread_mutex_t lock;
} sdb__log_module__;

int eit__write_log__(sdb__log_module__ *module, const char *log_level,
		     const char *fmt, ...)
	__attribute__((format(printf, 3, 4)));

#define EIT__LOG_LEVEL_CHECK__(level) (EIT_LOG_LEVEL >= EIT_LOG_LEVEL_##level)

#define EIT_LOGGING_NOT_USED                          \
	static sdb__log_module__ *eit__log_instance__ \
		__attribute__((used)) = NULL

#define EIT_LOG_REGISTER(module_name)                                         \
	static char EIT_CONCAT3(eit__log_module_, module_name,                \
				buf__)[EIT_LOG_BUF_SIZE];                     \
	sdb__log_module__ EIT_CONCAT3(eit__log_module_, module_name,          \
				      __) __attribute__((used)) = {           \
		.name = EIT_STRINGIFY(module_name),                           \
		.buf_size = EIT_LOG_BUF_SIZE,                                 \
		.buf = EIT_CONCAT3(eit__log_module_, module_name, buf__),     \
		.lock = PTHREAD_MUTEX_INITIALIZER                             \
	};                                                                    \
	static sdb__log_module__ *eit__log_instance__ __attribute__((used)) = \
		&EIT_CONCAT3(eit__log_module_, module_name, __)

#define EIT_LOG_DECLARE(name)                                                 \
	extern sdb__log_module__ EIT_CONCAT3(eit__log_module_, name, __);     \
	static sdb__log_module__ *eit__log_instance__ __attribute__((used)) = \
		&EIT_CONCAT3(eit__log_module_, name, __)

#define EIT__LOG__(log_level, fmt, ...)                                        \
	do {                                                                   \
		if (EIT__LOG_LEVEL_CHECK__(log_level)) {                       \
			int log_ret = eit__write_log__(                        \
				eit__log_instance__, EIT_STRINGIFY(log_level), \
				fmt, ##__VA_ARGS__);                           \
			assert(log_ret >= 0);                                  \
		}                                                              \
	} while (0)

#define EitLogDebug(...) EIT__LOG__(DBG, ##__VA_ARGS__)
#define EitLogInfo(...) EIT__LOG__(INF, ##__VA_ARGS__)
#define EitLogWarning(...) EIT__LOG__(WRN, ##__VA_ARGS__)
#define EitLogError(...) EIT__LOG__(ERR, ##__VA_ARGS__)

#if EIT_ASSERT == 1
#define EitAssert(condition, fmt, ...)                                       \
	do {                                                                 \
		if (!(condition)) {                                          \
			eit__write_log__(eit__log_instance__, "DBG",         \
					 "ASSERT (%s,%d): : " fmt, __func__, \
					 __LINE__, ##__VA_ARGS__);           \
			assert(condition);                                   \
		}                                                            \
	} while (0)
#else
#define EitAssert(...)
#endif

////////////////////////////////////////
//            MEM TRACE               //
////////////////////////////////////////

void *eit__malloc_trace__(size_t size, int line, const char *func,
			  sdb__log_module__ *module);
void *eit__calloc_trace__(size_t ElementCount, size_t Elementsize, int line,
			  const char *func, sdb__log_module__ *module);
void *eit__realloc_trace__(void *ptr, size_t size, int line, const char *func,
			   sdb__log_module__ *module);
void eit__free_trace__(void *ptr, int line, const char *func,
		       sdb__log_module__ *module);

#if SDB_MEM_TRACE == 1
#define malloc(size) \
	eit__malloc_trace__(size, __LINE__, __func__, eit__log_instance__)
#define calloc(Count, size)                                  \
	eit__calloc_trace__(count, size, __LINE__, __func__, \
			    eit__log_instance__)
#define realloc(ptr, size) \
	eit__realloc_trace__(ptr, size, __LINE__, __func__, eit__log_instance__)
#define free(ptr) \
	eit__free_trace__(ptr, __LINE__, __func__, eit__log_instance__)
#endif

/////////////////////////////////////////
//              FILE IO                //
/////////////////////////////////////////

typedef struct eit_file_data {
	size_t size;
	uint8_t *data;
} eit_file_data;

eit_file_data *eit_load_file_into_mem(const char *filename);
void eit_free_file_data(eit_file_data **file_data);
int eit_write_bytes_to_file(const uint8_t *buf, size_t size,
			    const char *filename);

#if defined(__cplusplus)
EIT_END_EXTERN_C
#endif
#endif // EIT_H_INCLUDE

// WARN: Only one file in a program should define SDB_H_IMPLEMENTATION, otherwise you will get
// redefintion errors
//#define EIT_H_IMPLEMENTATION // Uncomment/comment to enable/disable syntax highlighting
#ifdef EIT_H_IMPLEMENTATION

////////////////////////////////////////
//              LOGGING               //
////////////////////////////////////////
/* Based on the logging frontend I wrote for oec */

int eit__write_log__(sdb__log_module__ *module, const char *log_level,
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
//            MEM TRACE               //
////////////////////////////////////////

void *eit__malloc_trace__(size_t size, int line, const char *func,
			  sdb__log_module__ *module)
{
	void *ptr = malloc(size);
	eit__write_log__(module, "DBG", "MALLOC (%s,%d): %p (%zd B)", func,
			 line, ptr, size);
	return ptr;
}

void *eit__calloc_trace__(size_t count, size_t size, int line, const char *func,
			  sdb__log_module__ *module)
{
	void *ptr = calloc(count, size);
	eit__write_log__(module, "DBG",
			 "CALLOC (%s,%d): %p (%lu * %luB = %zd B)", func, line,
			 ptr, count, size, (count * size));
	return ptr;
}

void *eit__realloc_trace__(void *ptr, size_t size, int line, const char *func,
			   sdb__log_module__ *module)
{
	void *original = ptr;
	void *reallocd = NULL;
	if (ptr != NULL) {
		reallocd = realloc(ptr, size);
	}
	eit__write_log__(module, "DBG", "REALLOC (%s,%d): %p -> %p (%zd B)",
			 func, line, original, reallocd, size);
	return reallocd;
}

void eit__free_trace__(void *ptr, int line, const char *func,
		       sdb__log_module__ *module)
{
	eit__write_log__(module, "DBG", "FREE (%s,%d): %p", func, line, ptr);
	if (ptr != NULL) {
		free(ptr);
	}
}

#if (SDB_MEM_TRACE == 1) && (SDB_LOG_LEVEL >= SDB_LOG_LEVEL_DBG)
#define malloc(size) \
	eit__malloc_trace__(size, __LINE__, __func__, eit__log_instance__)
#define calloc(Count, size)                                  \
	eit__calloc_trace__(Count, size, __LINE__, __func__, \
			    eit__log_instance__)
#define realloc(ptr, size) \
	eit__realloc_trace__(ptr, size, __LINE__, __func__, eit__log_instance__)
#define free(ptr) \
	eit__free_trace__(ptr, __LINE__, __func__, eit__log_instance__)
#endif

eit_file_data *eit_load_file_into_mem(const char *filename)
{
	FILE *file = fopen(filename, "rb");
	if (!file) {
		fprintf(stderr, "Failed to open file: %s\n", strerror(errno));
		return NULL;
	}

	if (fseek(file, 0L, SEEK_END) != 0) {
		fprintf(stderr, "Failed to seek in file %s\n", filename);
		fclose(file);
		return NULL;
	}

	size_t file_size = (size_t)ftell(file);
	rewind(file);

	eit_file_data *file_data;
	size_t file_data_size = sizeof(eit_file_data) + file_size + 1;
	file_data = calloc(1, file_data_size);
	if (!file_data) {
		fprintf(stderr,
			"Failed to allocate memory for file data for file %s\n",
			filename);
		fclose(file);
		return NULL;
	}

	file_data->size = file_size;
	file_data->data = (uint8_t *)file_data + sizeof(eit_file_data);

	size_t bytes_read = fread(file_data->data, 1, file_size, file);
	if (bytes_read != file_size) {
		fprintf(stderr, "Failed to read data from file %s: %s\n",
			filename, strerror(errno));

		fclose(file);
		free(file_data);

		return NULL;
	}

	fclose(file);
	return file_data;
}

void eit_free_file_data(eit_file_data **file_data)
{
	if (file_data && *file_data) {
		free(*file_data);
		*file_data = NULL;
	} else {
		printf("WRN: Pointer to file data pointer or file data pointer was NULL\n");
	}
}

int eit_write_bytes_to_file(const uint8_t *buf, size_t size,
			    const char *filename)
{
	FILE *file = fopen(filename, "wb");
	if (!file) {
		fprintf(stderr, "Unable to open file %s\n", filename);
		return errno;
	}

	if (!(fwrite(buf, size, 1, file) == 1)) {
		fprintf(stderr, "Failed to write buffer to file %s: %s",
			filename, strerror(errno));
		return errno;
	}

	fclose(file);
	return 0;
}

// NOLINTEND(misc-definitions-in-headers)

#endif // EIT_H_IMPLEMENTATION
