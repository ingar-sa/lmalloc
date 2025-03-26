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

#define sdb_internal static
#define sdb_persist static
#define sdb_global static

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

/**
 * sdb_errno is a custom type to represent error and success codes in systems using eit_.
 * Values >= 0 indicate success, while values < 0 indicate errors.
 *
 * Generic success is defined as 0, and generic error is defined as -1.
 * Custom errno codes can be defined in the below enum.
 *
 * When appropriate, negative POSIX errno values can be returned,
 * e.g. -EINVAL.
 */
// TODO(ingar): Check number range for POSIX errno codes and make sure they don't overlap with ours
typedef int_least32_t sdb_errno;
enum {
	EITE_SUCCESS = 0,
	EITE_ERR = 1,

	EITE_DBS_UNAVAIL = 2,
	EITE_CP_UNAVAIL = 3,

	EITE_CONN_CLOSED_SUCS = 4,
	EITE_CONN_CLOSED_ERR = 5,

	EITE_PG_ERR = 6,

	EITE_JSON_ERR = 7,

	EITE_PTR_WAS_NULL = 8,
};

static inline const char *eit_StrErr(sdb_errno E)
{
	if (E < 0) {
		E = -E;
	}
	switch (E) {
	case EITE_SUCCESS:
		return "Success";
	case EITE_ERR:
		return "Error";
	case EITE_DBS_UNAVAIL:
		return "Database system unavailable";
	case EITE_CP_UNAVAIL:
		return "Communication protocol unavailable";
	case EITE_CONN_CLOSED_SUCS:
		return "Connection closed successfully";
	case EITE_CONN_CLOSED_ERR:
		return "Connection closed with error";
	case EITE_PG_ERR:
		return "PostgreSQL error";
	case EITE_JSON_ERR:
		return "JSON parsing error";
	case EITE_PTR_WAS_NULL:
		return "Pointer was NULL";
	default:
		return "Unknown error";
	}
}

#define EIT_EXPAND(x) x
#define EIT__STRINGIFY__(x) #x
#define EIT_STRINGIFY(x) EIT__STRINGIFY__(x)

#define EIT__CONCAT2__(x, y) x##y
#define EIT_CONCAT2(x, y) EIT__CONCAT2__(x, y)

#define EIT__CONCAT3__(x, y, z) x##y##z
#define EIT_CONCAT3(x, y, z) EIT__CONCAT3__(x, y, z)

#define eit_KiloByte(Number) (Number * 1000ULL)
#define eit_MegaByte(Number) (SdbKiloByte(Number) * 1000ULL)
#define eit_GigaByte(Number) (SdbMegaByte(Number) * 1000ULL)
#define eit_TeraByte(Number) (SdbGigaByte(Number) * 1000ULL)

#define eit_KibiByte(Number) (Number * 1024ULL)
#define eit_MebiByte(Number) (SdbKibiByte(Number) * 1024ULL)
#define eit_GibiByte(Number) (SdbMebiByte(Number) * 1024ULL)
#define eit_TebiByte(Number) (SdbGibiByte(Number) * 1024ULL)

#define eit_ArrayLen(Array) (sizeof(Array) / sizeof(Array[0]))

#define eit_Max(a, b) ((a > b) ? a : b)
#define eit_Min(a, b) ((a < b) ? a : b)

bool eit_DoubleEpsilonCompare(const double A, const double B);
u64 eit_DoubleSignBit(double F);
double eit_RadiansFromDegrees(double Degrees);
size_t eit_MemSizeFromString(const char *SizeStr);

////////////////////////////////////////
//              LOGGING               //
////////////////////////////////////////
/* Based on the logging frontend I wrote for oec */

#if !defined(EIT_LOG_BUF_SIZE)
#define EIT_LOG_BUF_SIZE 2048
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
#define eit_PrintfDebug(...) printf(__VA_ARGS__);
#else
#define eit_PrintfDebug(...)
#endif

typedef struct sdb__log_module__ {
	const char *Name;
	u64 BufferSize;
	char *Buffer;
	pthread_mutex_t Lock;
} sdb__log_module__;

i64 eit___WriteLog__(sdb__log_module__ *Module, const char *LogLevel,
		     const char *Fmt, ...)
	__attribute__((format(printf, 3, 4)));

#define EIT__LOG_LEVEL_CHECK__(level) (EIT_LOG_LEVEL >= EIT_LOG_LEVEL_##level)

#define EIT_LOGGING_NOT_USED                          \
	static sdb__log_module__ *eit___LogInstance__ \
		__attribute__((used)) = NULL

#define EIT_LOG_REGISTER(module_name)                                          \
	static char EIT_CONCAT3(eit___LogModule, module_name,                  \
				Buffer__)[EIT_LOG_BUF_SIZE];                   \
	sdb__log_module__ EIT_CONCAT3(eit___LogModule, module_name,            \
				      __) __attribute__((used)) = {            \
		.Name = EIT_STRINGIFY(module_name),                            \
		.BufferSize = EIT_LOG_BUF_SIZE,                                \
		.Buffer = EIT_CONCAT3(eit___LogModule, module_name, Buffer__), \
		.Lock = PTHREAD_MUTEX_INITIALIZER                              \
	};                                                                     \
	static sdb__log_module__ *eit___LogInstance__ __attribute__((used)) =  \
		&EIT_CONCAT3(eit___LogModule, module_name, __)

#define EIT_LOG_DECLARE(name)                                                 \
	extern sdb__log_module__ EIT_CONCAT3(eit___LogModule, name, __);      \
	static sdb__log_module__ *eit___LogInstance__ __attribute__((used)) = \
		&EIT_CONCAT3(eit___LogModule, name, __)

#define EIT__LOG__(log_level, fmt, ...)                                       \
	do {                                                                  \
		if (EIT__LOG_LEVEL_CHECK__(log_level)) {                      \
			sdb_errno LogRet = eit___WriteLog__(                  \
				Sdb__LogInstance__, EIT_STRINGIFY(log_level), \
				fmt, ##__VA_ARGS__);                          \
			assert(LogRet >= 0);                                  \
		}                                                             \
	} while (0)

#define eit_LogDebug(...) EIT__LOG__(DBG, ##__VA_ARGS__)
#define eit_LogInfo(...) EIT__LOG__(INF, ##__VA_ARGS__)
#define eit_LogWarning(...) EIT__LOG__(WRN, ##__VA_ARGS__)
#define eit_LogError(...) EIT__LOG__(ERR, ##__VA_ARGS__)

#if EIT_ASSERT == 1
#define eit_Assert(condition, fmt, ...)                                    \
	do {                                                               \
		if (!(condition)) {                                        \
			eit___WriteLog__(Sdb__LogInstance__, "DBG",        \
					 "ASSERT (%s,%d): " fmt, __func__, \
					 __LINE__, ##__VA_ARGS__);         \
			assert(condition);                                 \
		}                                                          \
	} while (0)
#else
#define eit_Assert(...)
#endif
////////////////////////////////////////
//               MEMORY               //
////////////////////////////////////////

typedef struct sdb_slice {
	u64 Len;
	u64 ESize;
	u8 *Mem;
} sdb_slice;

typedef struct sdb_arena {
	u64 Cur;
	u64 Cap;
	u8 *Mem;

} sdb_arena;

typedef struct {
	sdb_arena *Arena;
	u64 F5;

} sdb_scratch_arena;

typedef struct {
	sdb_arena **Arenas;
	u64 Count;
	u64 MaxCount;

} sdb__thread_arenas__;

#define EIT_THREAD_ARENAS_REGISTER(thread_name, count)                       \
	static __thread sdb_arena *EIT_CONCAT3(eit___ThreadArenas,           \
					       thread_name, Buffer__)[count] \
		__attribute__((used));                                       \
	__thread sdb__thread_arenas__ EIT_CONCAT3(eit___ThreadArenas,        \
						  thread_name, __)           \
		__attribute__((used)) = { .Arenas = NULL,                    \
					  .Count = 0,                        \
					  .MaxCount = count };               \
	static __thread sdb__thread_arenas__ *eit___ThreadArenasInstance__   \
		__attribute__((used))

#define EIT_THREAD_ARENAS_EXTERN(thread_name)                                \
	extern __thread sdb__thread_arenas__ EIT_CONCAT3(eit___ThreadArenas, \
							 thread_name, __);   \
	static __thread sdb__thread_arenas__ *eit___ThreadArenasInstance__   \
		__attribute__((used))

// WARN: Must be used at runtime, not compile time. This is because you cannot take the address
// of a __thread varible in static initialization, since the address will differ per thread.
void eit___ThreadArenasInit__(sdb_arena *TABuf[], sdb__thread_arenas__ *TAs,
			      sdb__thread_arenas__ **TAInstance);
#define eit_ThreadArenasInit(thread_name)                              \
	eit___ThreadArenasInit__(                                      \
		EIT_CONCAT3(Sdb__ThreadArenas, thread_name, Buffer__), \
		&EIT_CONCAT3(eit___ThreadArenas, thread_name, __),     \
		&eit___ThreadArenasInstance__)

void eit___ThreadArenasInitExtern__(sdb__thread_arenas__ *TAs,
				    sdb__thread_arenas__ **TAInstance);
#define eit_ThreadArenasInitExtern(thread_name)                        \
	eit___ThreadArenasInitExtern__(&EIT_CONCAT3(Sdb__ThreadArenas, \
						    thread_name, __),  \
				       &eit___ThreadArenasInstance__)

sdb_errno eit___ThreadArenasAdd__(sdb_arena *Arena,
				  sdb__thread_arenas__ *TAInstance);
#define eit_ThreadArenasAdd(arena) \
	Sdb__ThreadArenasAdd__(arena, Sdb__ThreadArenasInstance__)

sdb_scratch_arena eit_ScratchBegin(sdb_arena *Arena);

sdb_scratch_arena eit___ScratchGet__(sdb_arena **Conflicts, u64 ConflictCount,
				     sdb__thread_arenas__ *TAs);
#define eit_ScratchGet(conflicts, conflict_count)     \
	eit___ScratchGet__(conflicts, conflict_count, \
			   Sdb__ThreadArenasInstance__)

void eit___ScratchRelease__(sdb_scratch_arena Scratch);
#define eit_ScratchRelease(scratch) Sdb__ScratchRelease__(scratch)

sdb_scratch_arena eit_ScratchBegin(sdb_arena *Arena);

void eit_MemZero(void *Mem, u64 Size);
void eit_MemZeroSecure(void *Mem, u64 Size);
#define eit_MemZeroStruct(struct) SdbMemZero(struct, sizeof(*struct))
#define eit_MemZeroStructSecure(struct) \
	SdbMemZeroSecure(struct, sizeof(*struct))

void *eit_Memcpy(void *__restrict To, const void *__restrict From, size_t Len);
void *eit_Memset(void *__restrict Data, int SetTo, size_t Len);
bool eit_Memcmp(const void *Lhs, const void *Rhs, size_t Len);

void eit_ArenaInit(sdb_arena *Arena, u8 *Mem, u64 Size);
sdb_arena *eit_ArenaBootstrap(sdb_arena *Arena, sdb_arena *NewArena_, u64 Size);

void *eit_ArenaPush(sdb_arena *Arena, u64 Size);
void *eit_ArenaPushZero(sdb_arena *Arena, u64 Size);
void *eit_ArenaPop(sdb_arena *Arena, u64 Size);

u64 eit_ArenaRemaining(sdb_arena *Arena);
u64 eit_ArenaGetPos(sdb_arena *Arena);
void *eit_ArenaSeek(sdb_arena *Arena, u64 Pos);
u64 eit_ArenaReserve(sdb_arena *Arena, u64 Size);

void eit_ArenaClear(sdb_arena *Arena);
void eit_ArenaClearZero(sdb_arena *Arena);

#define eit_PushArray(arena, type, count) \
	(type *)SdbArenaPush(arena, sizeof(type) * (count))
#define eit_PushArrayZero(arena, type, count) \
	(type *)SdbArenaPushZero(arena, sizeof(type) * (count))

#define eit_PushStruct(arena, type) SdbPushArray(arena, type, 1)
#define eit_PushStructZero(arena, type) SdbPushArrayZero(arena, type, 1)

void eit_ArrayShift(void *Mem, u64 From, u64 To, u64 Count, u64 ElementSize);
#define eit_ArrayDeleteAndShift(mem, i, count, esize) \
	SdbArrayShift(mem, (i + 1), i, count, esize)

#define EIT_POOL_ALLOCATOR_DEFINE(type_name, func_name)                      \
	typedef struct type_name##_pool {                                    \
		sdb_arena *Arena;                                            \
		type_name *FirstFree;                                        \
	} type_name##_pool;                                                  \
                                                                             \
	type_name *func_name##Alloc(type_name##_pool *Pool)                  \
	{                                                                    \
		type_name *Result = Pool->FirstFree;                         \
		if (Result) {                                                \
			Pool->FirstFree = Pool->FirstFree->Next;             \
			eit_MemZeroStruct(Result);                           \
		} else {                                                     \
			Result = eit_PushStructZero(Pool->Arena, type_name); \
		}                                                            \
                                                                             \
		return Result;                                               \
	}                                                                    \
                                                                             \
	void func_name##Release(type_name##_pool *Pool, type_name *Instance) \
	{                                                                    \
		Instance->Next = Pool->FirstFree;                            \
		Pool->FirstFree = Instance;                                  \
	}

////////////////////////////////////////
//              STRINGS               //
////////////////////////////////////////

// NOTE(ingar): These are libc equivalents and are intended to be used with regular C-strings, not
// sdb_strings
u64 eit_Strnlen(const char *String, u64 Max);
u64 eit_Strlen(const char *String);
char *eit_Strdup(char *String, sdb_arena *Arena);

// Based on ginger bills gbString implementation.
//
// A few assumptions being made:
// The strings are not intended to be reallocated at a new address. If something has been pushed
// onto the arena, and you use a function that increases the strings length or capacity, it will
// *NOT* work (at least for the moment).
//
// The way it works is that sdb_string functions return a pointer to to the C-string, which is
// always null-terminated, while an "invisible" header precedes it in memory. The advantage of this
// is that an sdb_string can be passed to any function that accepts a C-string, and it will work
// correctly. This avoids the typical "improved strings" pattern of having to access the string a la
// custom_string->cstring. This improves the ergonomics of sdb_strings, and also improves cache
// behavior since the header and the C-string are always contiguous in memory.
typedef char *sdb_string;
typedef struct {
	sdb_arena *Arena;
	u64 Len;
	u64 Cap;

} sdb_string_header;

#define EIT_STRING_HEADER(str) ((sdb_string_header *)(str) - 1)

u64 eit_StringLen(sdb_string String);
u64 eit_StringCap(sdb_string String);
u64 eit_StringAvailableSpace(sdb_string String);
u64 eit_StringAllocSize(sdb_string String);

sdb_string eit_StringMakeSpace(sdb_string String, u64 AddLen);
sdb_string eit_StringMake(sdb_arena *A, const char *String);
void eit_StringFree(sdb_string String);

sdb_string eit_StringDuplicate(sdb_arena *A, const sdb_string String);
void eit_StringClear(sdb_string String);
sdb_string eit_StringSet(sdb_string String, const char *CString);
void eit_StringBackspace(sdb_string String, u64 N);

sdb_string eit_StringAppend(sdb_string String, sdb_string Other);
sdb_string eit_StringAppendC(sdb_string String, const char *Other);
sdb_string eit_StringAppendFmt(sdb_string String, const char *Fmt, ...)
	__attribute__((format(printf, 2, 3)));

bool eit_StringsAreEqual(sdb_string Lhs, sdb_string Rhs);

////////////////////////////////////////
//                RNG                 //
////////////////////////////////////////

// Implementation of the PCG algorithm (https://www.pcg-random.org)
// It's the caller's responsibilites to have called SeedRandPCG before use
// WARN: The pcg state is statically allocated inside eit___GetPCGState__, and is not protected by a
// lock, which means race conditions can occurr if multiple threads use it! (Though maybe this
// actually improves unpredictability?)

u32 *eit___GetPCGState__(void);
u32 eit_RandPCG(void);
void eit_SeedRandPCG(uint32_t Seed);

/////////////////////////////////////////
//              FILE IO                //
/////////////////////////////////////////

typedef struct {
	u64 Size;
	u8 Data[];
} sdb_file_data;

sdb_file_data *eit_LoadFileIntoMemory(const char *Filename, sdb_arena *Arena);
bool eit_WriteBufferToFile(void *Buffer, u64 ElementSize, u64 ElementCount,
			   const char *Filename);
bool eit_Write_sdb_file_data_ToFile(sdb_file_data *FileData,
				    const char *Filename);

typedef struct {
	void *Data; // Mapped memory region
	size_t Size; // Size of mapping
	int Fd; // File descriptor (kept open if requested)
	sdb_string Filename; // Copy of filename for cleanup
	int Flags; // Saved flags for cleanup
} sdb_mmap;

/**
 * @brief Creates a memory mapping with optional file backing
 *
 * This function provides a convenient wrapper around mmap() with integrated file handling.
 * It can create both file-backed and anonymous mappings, with various protection and
 * sharing options.
 *
 * @param Map      Pointer to the sdb_mmap that will be filled
 * @param MAddr    Desired mapping address (NULL for system choice)
 * @param MSize    Size of the mapping in bytes
 * @param MProt    Memory protection flags (can be combined with |):
 *                 - PROT_NONE  : No access permitted
 *                 - PROT_READ  : Pages can be read
 *                 - PROT_WRITE : Pages can be written
 *                 - PROT_EXEC  : Pages can be executed
 *
 * @param MFlags   Mapping flags (can be combined with |):
 *                 - MAP_SHARED    : Updates visible to other processes
 *                 - MAP_PRIVATE   : Creates copy-on-write mapping
 *                 - MAP_FIXED     : Use exact address (not recommended)
 *                 - MAP_ANONYMOUS : Create anonymous mapping (no file backing)
 *                 - MAP_LOCKED    : Lock pages into memory
 *                 - MAP_POPULATE  : Populate page tables
 *                 - MAP_NORESERVE : No swap space reservation
 *                 - MAP_HUGETLB   : Use huge pages
 *
 * @param MFd      File descriptor for mapping:
 *                 - >= 0 : Use this file descriptor
 *                 - -1   : Used with MAP_ANONYMOUS
 *                 - < -1 : Open file using FileName, OFlags, and OMode
 *
 * @param MOffset  Offset into file (must be page-aligned)
 *
 * @param FileName Path to file (used only if MFd < -1)
 *
 * @param OFlags   File open flags (used only if MFd < -1), can combine with |:
 *                 Required - one of:
 *                 - O_RDONLY  : Read only
 *                 - O_WRONLY  : Write only
 *                 - O_RDWR    : Read and write
 *
 *                 Optional:
 *                 - O_APPEND    : Append to file
 *                 - O_CREAT     : Create if not exists
 *                 - O_EXCL      : With O_CREAT: fail if exists
 *                 - O_TRUNC     : Truncate to MSize (WARN: Will shrink the file if the file's size
 * is > MSize)
 *                 - O_DIRECT    : Direct I/O
 *                 - O_SYNC      : Synchronous I/O
 *                 - O_NOFOLLOW  : Don't follow symlinks
 *                 - O_CLOEXEC   : Close on exec
 *                 - O_TMPFILE   : Create unnamed temporary file
 *
 * @param OMode    File creation mode (used only if OFlags includes O_CREAT):
 *                 Typical values (in octal):
 *                 - 0644 : User: rw-, Group: r--, Other: r--
 *                 - 0666 : User: rw-, Group: rw-, Other: rw-
 *                 - 0600 : User: rw-, Group: ---, Other: ---
 *                 - 0755 : User: rwx, Group: r-x, Other: r-x
 *
 *                 Individual bits:
 *                 - S_IRUSR (0400) : User read
 *                 - S_IWUSR (0200) : User write
 *                 - S_IXUSR (0100) : User execute
 *                 - S_IRGRP (0040) : Group read
 *                 - S_IWGRP (0020) : Group write
 *                 - S_IXGRP (0010) : Group execute
 *                 - S_IROTH (0004) : Other read
 *                 - S_IWOTH (0002) : Other write
 *                 - S_IXOTH (0001) : Other execute
 *
 *                 Special bits:
 *                 - S_ISUID (04000) : Set UID bit
 *                 - S_ISGID (02000) : Set GID bit
 *                 - S_ISVTX (01000) : Sticky bit
 *
 * @return sdb_errno:
 *         - 0 on success
 *         - Negative errno value on failure:
 *           - -EACCES  : Permission denied
 *           - -EEXIST  : File exists (with O_CREAT | O_EXCL)
 *           - -EINVAL  : Invalid arguments
 *           - -ENFILE  : System file table full
 *           - -ENOMEM  : Out of memory
 *           - -ENOENT  : File not found
 *
 * @note
 * - The Map structure must be allocated by the caller
 * - FileName string must remain valid throughout the mapping's lifetime
 * - File descriptor handling:
 *   - MAP_SHARED: fd kept open
 *   - MAP_PRIVATE: fd closed after mapping
 *   - MAP_ANONYMOUS: fd closed/ignored
 * - The actual mapping size will be rounded up to the system page size
 * - MProt permissions must be compatible with OFlags
 * - The actual protection may be more restrictive than MProt due to file permissions
 * - MAP_FIXED is dangerous and should be avoided
 * - File permissions are masked by the process umask
 * - Some combinations may not be supported on all systems
 */
sdb_errno eit___MemMap__(sdb_mmap *Map, void *MAddr, size_t MSize, int MProt,
			 int MFlags, int MFd, off_t MOffset,
			 sdb_string FileName, int OFlags, mode_t OMode,
			 sdb__log_module__ *Module);

#define eit_MemMap(map, maddr, msize, mprot, mflags, mfd, moffset, filename, \
		   oflags, omode)                                            \
	eit___MemMap__(map, maddr, msize, mprot, mflags, mfd, moffset,       \
		       filename, oflags, omode, eit___LogInstance__)

void eit_MemUnmap(sdb_mmap *Map);
int eit_MemMapSync(sdb_mmap *Map);
int eit_MemMapAdvise(sdb_mmap *Map, int Advice);

////////////////////////////////////////
//            "TOKENIZER"             //
////////////////////////////////////////

typedef struct {
	char *Start;
	u64 Len;

} sdb_token;

sdb_token eit_GetNextToken(char **Cursor);

////////////////////////////////////////
//            MEM TRACE               //
////////////////////////////////////////

void *eit___MallocTrace__(size_t Size, int Line, const char *Func,
			  sdb__log_module__ *Module);
void *eit___CallocTrace__(u64 ElementCount, u64 ElementSize, int Line,
			  const char *Func, sdb__log_module__ *Module);
void *eit___ReallocTrace__(void *Pointer, size_t Size, int Line,
			   const char *Func, sdb__log_module__ *Module);
void eit___FreeTrace__(void *Pointer, int Line, const char *Func,
		       sdb__log_module__ *Module);

#if EIT_MEM_TRACE == 1
#define malloc(Size) \
	eit___MallocTrace__(Size, __LINE__, __func__, Sdb__LogInstance__)
#define calloc(Count, Size) \
	eit___CallocTrace__(Count, Size, __LINE__, __func__, Sdb__LogInstance__)
#define realloc(Pointer, Size)                                  \
	eit___ReallocTrace__(Pointer, Size, __LINE__, __func__, \
			     Sdb__LogInstance__)
#define free(Pointer) \
	eit___FreeTrace__(Pointer, __LINE__, __func__, Sdb__LogInstance__)
#endif

#if defined(__cplusplus)
EIT_END_EXTERN_C
#endif

#endif // EIT_H_INCLUDE

// WARN: Only one file in a program should define EIT_H_IMPLEMENTATION, otherwise you will get
// redefintion errors
// #define EIT_H_IMPLEMENTATION
#ifdef EIT_H_IMPLEMENTATION

// NOTE(ingar): The reason this is done is so that functions inside eit_.h don't use the trace
// macros. The macros are redefined at the bottom
#if EIT_MEM_TRACE == 1
#undef malloc
#undef calloc
#undef realloc
#undef free
#endif

////////////////////////////////////////
//               MISC                 //
////////////////////////////////////////

bool eit_DoubleEpsilonCompare(const double A, const double B)
{
	double GreatestValue = (fabs(A) < fabs(B)) ? fabs(B) : fabs(A);
	return fabs(A - B) <= (GreatestValue * DBL_EPSILON);
}

u64 eit_DoubleSignBit(double F)
{
	u64 Mask = 1ULL << 63;
	u64 *Comp = (u64 *)&F;

	return (*Comp) & Mask;
}

double eit_RadiansFromDegrees(double Degrees)
{
	double Radians = 0.01745329251994329577f * Degrees;
	return Radians;
}

size_t eit_MemSizeFromString(const char *SizeStr)
{
	if (!SizeStr || !*SizeStr) {
		return 0;
	}

	char *EndPtr;
	size_t Value = strtoull(SizeStr, &EndPtr, 10);

	if (EndPtr == SizeStr) {
		return 0; // No number found
	}

	while (isspace(*EndPtr)) {
		EndPtr++;
	}

	if (!*EndPtr) {
		return Value;
	}

	size_t Multiplier = 0;
	switch (*EndPtr) {
	case 'T':
		Multiplier = 1000ULL * 1000ULL * 1000ULL * 1000ULL;
		break;
	case 't':
		Multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
		break;
	case 'G':
		Multiplier = 1000ULL * 1000ULL * 1000ULL;
		break;
	case 'g':
		Multiplier = 1024ULL * 1024ULL * 1024ULL;
		break;
	case 'M':
		Multiplier = 1000ULL * 1000ULL;
		break;
	case 'm':
		Multiplier = 1024ULL * 1024ULL;
		break;
	case 'K':
		Multiplier = 1000ULL;
		break;
	case 'k':
		Multiplier = 1024ULL;
		break;
	case 'B':
		// Check if this is just 'B' or part of a larger unit
		if (EndPtr[1] != '\0') {
			return 0;
		} else {
			Multiplier = 1;
		}
		break;
	default:
		return 0;
	}

	// If we have a unit letter, verify it's followed by 'B' (except for plain 'B')
	if (*EndPtr != 'B' && (EndPtr[1] != 'B' || EndPtr[2] != '\0')) {
		return 0;
	}

	return Value * Multiplier;
}

////////////////////////////////////////
//              LOGGING               //
////////////////////////////////////////
/* Based on the logging frontend I wrote for oec */

i64 eit___WriteLog__(sdb__log_module__ *Module, const char *LogLevel,
		     const char *Fmt, ...)
{
	if (Module == NULL) {
		return 0; // NOTE(ingar): If the log module is NULL, then we assume logging isn't used
	}

	pthread_mutex_lock(&Module->Lock);

	time_t PosixTime;
	struct tm TimeInfo;

	if ((time_t)(-1) == time(&PosixTime)) {
		return -errno;
	}

	if (NULL == localtime_r(&PosixTime, &TimeInfo)) {
		return -errno;
	}

	u64 CharsWritten = 0;
	u64 BufferRemaining = Module->BufferSize;

	int FormatRet =
		strftime(Module->Buffer, BufferRemaining, "%T: ", &TimeInfo);
	if (0 == FormatRet) {
		// NOTE(ingar): Since the buffer size is at least 128, this should never happen
		assert(FormatRet);
		return -ENOMEM;
	}

	CharsWritten = FormatRet;
	BufferRemaining -= FormatRet;

	FormatRet = snprintf(Module->Buffer + CharsWritten, BufferRemaining,
			     "%s: %s: ", Module->Name, LogLevel);
	if (FormatRet < 0) {
		return -errno;
	} else if ((u64)FormatRet >= BufferRemaining) {
		// NOTE(ingar): If the log module name is so long that it does not fit in 128 bytes - the
		// time stamp, it should be changed
		assert(FormatRet);
		return -ENOMEM;
	}

	CharsWritten += FormatRet;
	BufferRemaining -= FormatRet;

	va_list VaArgs;
	va_start(VaArgs, Fmt);
	FormatRet = vsnprintf(Module->Buffer + CharsWritten, BufferRemaining,
			      Fmt, VaArgs);
	va_end(VaArgs);

	if (FormatRet < 0) {
		return -errno;
	} else if ((u64)FormatRet >= BufferRemaining) {
		(void)eit_Memset(Module->Buffer + CharsWritten, 0,
				 BufferRemaining);
		FormatRet = snprintf(Module->Buffer + CharsWritten,
				     BufferRemaining, "%s",
				     "Message dropped; too big");
		if (FormatRet < 0) {
			return -errno;
		} else if ((u64)FormatRet >= BufferRemaining) {
			assert(FormatRet);
			return -ENOMEM;
		}
	}

	CharsWritten += FormatRet;
	Module->Buffer[CharsWritten++] = '\n';

	int OutFd;
	if ('E' == LogLevel[0]) {
		OutFd = STDERR_FILENO;
	} else {
		OutFd = STDOUT_FILENO;
	}

	if ((ssize_t)(-1) == write(OutFd, Module->Buffer, CharsWritten)) {
		return -errno;
	}

	pthread_mutex_unlock(&Module->Lock);

	return 0;
}

////////////////////////////////////////
//               MEMORY               //
////////////////////////////////////////

void *eit_Memcpy(void *__restrict To, const void *__restrict From, size_t Len)
{
	// NOTE(ingar): The compiler should replace this with memcpy if it's available
	for (size_t i = 0; i < Len; ++i) {
		((u8 *)To)[i] = ((u8 *)From)[i];
	}

	return To;
}

void *eit_Memset(void *__restrict Data, int SetTo, size_t Len)
{
	// NOTE(ingar): The compiler should replace this with memset if it's available. Don't ask me why
	// you cast it to a u8* and not an int*, but that must be done for it to be replaced
	for (size_t i = 0; i < Len; ++i) {
		((u8 *)Data)[i] = SetTo;
	}
	return Data;
}

bool eit_Memcmp(const void *Lhs, const void *Rhs, size_t Len)
{
	// NOTE(ingar): The compiler (probably) won't replace this one with memcmp, but GCC didn't
	// replace their own memcmp implementation with a call to memcmp, so that's just on them
	for (size_t i = 0; i < Len; ++i) {
		if (((u8 *)Lhs)[i] != ((u8 *)Rhs)[i]) {
			return false;
		}
	}
	return true;
}

void eit_MemZero(void *__restrict Mem, u64 Size)
{
	(void)eit_Memset(Mem, 0, Size);
}

void /* From https://github.com/BLAKE2/BLAKE2/blob/master/ref/blake2-impl.h */
eit_MemZeroSecure(void *Mem, u64 Size)
{
	// NOTE(ingar): This prevents the compiler from optimizing away memset. This could be replaced
	// by a call to memset_s if one uses C11, but this method should be compatible with earlier C
	// standards
	static void *(*const volatile memset_v)(void *, int, u64) = &memset;
	(void)memset_v(Mem, 0, Size);
}

void eit_ArenaInit(sdb_arena *Arena, u8 *Mem, u64 Size)
{
	Arena->Cur = 0;
	Arena->Cap = Size;
	Arena->Mem = Mem;
}

sdb_arena *eit_ArenaBootstrap(sdb_arena *Arena, sdb_arena *NewArena_, u64 Size)
{
	sdb_arena *NewArena;
	if (NewArena_ == NULL) {
		NewArena = eit_PushStruct(Arena, sdb_arena);
	} else {
		NewArena = NewArena_;
	}
	u8 *NewArenaMem = eit_PushArray(Arena, u8, Size);

	if (NULL == NewArena || NULL == NewArenaMem) {
		return NULL;
	}

	eit_ArenaInit(NewArena, NewArenaMem, Size);
	return NewArena;
}

// TODO(ingar): Create aligning pushes
void *eit_ArenaPush(sdb_arena *Arena, u64 Size)
{
	if (Arena->Cur + Size <= Arena->Cap) {
		u8 *AllocedMem = Arena->Mem + Arena->Cur;
		Arena->Cur += Size;
		return AllocedMem;
	}

	return NULL;
}

void *eit_ArenaPushZero(sdb_arena *Arena, u64 Size)
{
	if (Arena->Cur + Size <= Arena->Cap) {
		u8 *AllocedMem = Arena->Mem + Arena->Cur;
		Arena->Cur += Size;
		eit_MemZero(AllocedMem, Size);
		return AllocedMem;
	}

	return NULL;
}

u64 eit_ArenaRemaining(sdb_arena *Arena)
{
	return Arena->Cap - Arena->Cur;
}

// TODO(ingar): Does returning the position add overhead/is it necessary?
void *eit_ArenaPop(sdb_arena *Arena, u64 Size)
{
	if (Arena->Cur >= Size) {
		Arena->Cur -= Size;
		return Arena->Mem + Arena->Cur;
	}

	return NULL;
}

u64 eit_ArenaGetPos(sdb_arena *Arena)
{
	u64 Pos = Arena->Cur;
	return Pos;
}

void *eit_ArenaSeek(sdb_arena *Arena, u64 Pos)
{
	if (Pos <= Arena->Cap) {
		Arena->Cur = Pos;
		return Arena->Mem + Arena->Cur;
	}

	return NULL;
}

u64 eit_ArenaReserve(sdb_arena *Arena, u64 Size)
{
	if (Arena->Cur + Size <= Arena->Cap) {
		Arena->Cur += Size;
		return Size;
	} else {
		return eit_ArenaRemaining(Arena);
	}
}

void eit_ArenaClear(sdb_arena *Arena)
{
	Arena->Cur = 0;
}

void eit_ArenaClearZero(sdb_arena *Arena)
{
	eit_MemZero(Arena->Mem, Arena->Cap);
	Arena->Cur = 0;
}

void eit___ThreadArenasInit__(sdb_arena *TABuf[], sdb__thread_arenas__ *TAs,
			      sdb__thread_arenas__ **TAInstance)
{
	TAs->Arenas = TABuf;
	*TAInstance = TAs;
}

void eit___ThreadArenasInitExtern__(sdb__thread_arenas__ *TAs,
				    sdb__thread_arenas__ **TAInstance)
{
	*TAInstance = TAs;
}

sdb_errno eit___ThreadArenasAdd__(sdb_arena *Arena,
				  sdb__thread_arenas__ *TAInstance)
{
	if (TAInstance->Count + 1 <= TAInstance->MaxCount) {
		TAInstance->Arenas[TAInstance->Count++] = Arena;
		return 0;
	}

	return -1;
}

sdb_scratch_arena eit_ScratchBegin(sdb_arena *Arena)
{
	sdb_scratch_arena Scratch;
	Scratch.Arena = Arena;
	Scratch.F5 = eit_ArenaGetPos(Arena);
	return Scratch;
}

sdb_scratch_arena eit___ScratchGet__(sdb_arena **Conflicts, u64 ConflictCount,
				     sdb__thread_arenas__ *TAs)
{
	sdb_arena *Arena = NULL;
	if (ConflictCount > 0) {
		for (u64 i = 0; i < TAs->Count; ++i) {
			for (u64 j = 0; j < ConflictCount; ++j) {
				if (TAs->Arenas[i] != Conflicts[j]) {
					Arena = TAs->Arenas[i];
					goto exit;
				}
			}
		}
	} else {
		Arena = TAs->Arenas[0];
	}

exit:
	return (Arena == NULL) ? (sdb_scratch_arena){ 0 } :
				 eit_ScratchBegin(Arena);
}

void eit___ScratchRelease__(sdb_scratch_arena Scratch)
{
	eit_ArenaSeek(Scratch.Arena, Scratch.F5);
}

void eit_ArrayShift(void *Mem, u64 From, u64 To, u64 Count, u64 ElementSize)
{
	if (From != To) {
		u8 *Array = (u8 *)Mem;
		u8 *Src = Array + (From * ElementSize);
		u8 *Dst = Array + (To * ElementSize);

		u64 NumElements = (Count > From) ? (Count - From) : 0;

		if (NumElements > 0) {
			u64 NBytes = NumElements * ElementSize;
			memmove(Dst, Src, NBytes);
		}
	}
}

////////////////////////////////////////
//              STRINGS               //
////////////////////////////////////////

u64 eit_Strnlen(const char *String, u64 Max)
{
	u64 Count = 0;
	while (*String++ != '\0' && Count < Max) {
		++Count;
	}

	return Count;
}

u64 eit_Strlen(const char *String)
{
	u64 Count = 0;
	while (*String++ != '\0') {
		++Count;
	}

	return Count;
}

char *eit_Strdup(char *String, sdb_arena *Arena)
{
	u64 StringLength = eit_Strlen(String);
	char *NewString = eit_PushArray(Arena, char, StringLength + 1);
	eit_Memcpy(NewString, String, StringLength);
	NewString[StringLength] = '\0';

	return NewString;
}

//
// EIT String
//

u64 eit_StringLen(sdb_string String)
{
	return EIT_STRING_HEADER(String)->Len;
}
u64 eit_StringCap(sdb_string String);

u64 eit_StringCap(sdb_string String)
{
	return EIT_STRING_HEADER(String)->Cap;
}

u64 eit_StringAvailableSpace(sdb_string String)
{
	sdb_string_header *Header = EIT_STRING_HEADER(String);
	return Header->Cap - Header->Len;
}

u64 eit_StringAllocSize(sdb_string String)
{
	u64 Size = sizeof(sdb_string_header) + eit_StringCap(String);
	return Size;
}

void eit___StringSetLen__(sdb_string String, u64 Len)
{
	EIT_STRING_HEADER(String)->Len = Len;
}

void eit___StringSetCap__(sdb_string String, u64 Cap)
{
	EIT_STRING_HEADER(String)->Cap = Cap;
}

sdb_string eit___StringMake__(sdb_arena *A, const void *InitString, u64 Len)
{
	size_t HeaderSize = sizeof(sdb_string_header);
	void *Ptr = eit_ArenaPush(A, HeaderSize + Len + 1);
	if (Ptr == NULL) {
		return NULL;
	}
	if (InitString == NULL) {
		eit_Memset(Ptr, 0, HeaderSize + Len + 1);
	}

	sdb_string String;
	sdb_string_header *Header;

	String = (char *)Ptr + HeaderSize;
	Header = EIT_STRING_HEADER(String);

	Header->Arena = A;
	Header->Len = Len;
	Header->Cap = Len;
	if (Len > 0 && (InitString != NULL)) {
		eit_Memcpy(String, InitString, Len);
		String[Len] = '\0';
	}

	return String;
}

sdb_string eit_StringMake(sdb_arena *A, const char *String)
{
	u64 Len = (String != NULL) ? eit_Strlen(String) : 0;
	return eit___StringMake__(A, String, Len);
}

void eit_StringFree(sdb_string String)
{
	if (String != NULL) {
		sdb_string_header *H = EIT_STRING_HEADER(String);
		u64 PopSize = sizeof(sdb_string_header) + H->Cap + 1;
		eit_ArenaPop(H->Arena, PopSize);
	}
}

sdb_string eit_StringDuplicate(sdb_arena *A, const sdb_string String)
{
	return eit___StringMake__(A, String, SdbStringLen(String));
}

void eit_StringClear(sdb_string String)
{
	eit___StringSetLen__(String, 0);
	String[0] = '\0';
}

void eit_StringBackspace(sdb_string String, u64 N)
{
	u64 NewLen = eit_StringLen(String) - N;
	eit___StringSetLen__(String, NewLen);
	String[NewLen] = '\0';
}

sdb_string eit_StringMakeSpace(sdb_string String, u64 AddLen)
{
	u64 Available = eit_StringAvailableSpace(String);
	if (Available < AddLen) {
		sdb_arena *A = EIT_STRING_HEADER(String)->Arena;
		u64 Reserved = eit_ArenaReserve(A, AddLen);
		if (Reserved < AddLen) {
			return NULL;
		}

		u64 NewCap = eit_StringCap(String) + AddLen;
		eit___StringSetCap__(String, NewCap);
	}
	return String;
}

sdb_string eit___StringAppend__(sdb_string String, const void *Other,
				u64 OtherLen)
{
	String = eit_StringMakeSpace(String, OtherLen);
	if (String == NULL) {
		return NULL;
	}

	u64 CurLen = eit_StringLen(String);
	eit_Memcpy(String + CurLen, Other, OtherLen);
	eit___StringSetLen__(String, CurLen + OtherLen);
	String[CurLen + OtherLen] = '\0';
	return String;
}

sdb_string eit_StringAppend(sdb_string String, sdb_string Other)
{
	return eit___StringAppend__(String, Other, SdbStringLen(Other));
}

sdb_string eit_StringAppendC(sdb_string String, const char *Other)
{
	return eit___StringAppend__(String, Other, SdbStrlen(Other));
}

sdb_string eit_StringAppendFmt(sdb_string String, const char *Fmt, ...)
{
	// TODO(ingar): Make arena mandatory or use String's arena?
	char Buf[1024] = { 0 };
	va_list Va;
	va_start(Va, Fmt);
	u64 FmtLen = vsnprintf(Buf, eit_ArrayLen(Buf) - 1, Fmt,
			       Va); // TODO(ingar): stb_sprintf?
	va_end(Va);
	return eit___StringAppend__(String, Buf, FmtLen);
}

sdb_string eit_StringSet(sdb_string String, const char *CString)
{
	u64 Len = eit_Strlen(CString);
	String = eit_StringMakeSpace(String, Len);
	if (String == NULL) {
		return NULL;
	}

	eit_Memcpy(String, CString, Len);
	String[Len] = '\0';
	eit___StringSetLen__(String, Len);

	return String;
}

bool eit_StringsAreEqual(sdb_string Lhs, sdb_string Rhs)
{
	u64 LStringLen = eit_StringLen(Lhs);
	u64 RStringLen = eit_StringLen(Rhs);

	if (LStringLen != RStringLen) {
		return false;
	} else {
		return eit_Memcmp(Lhs, Rhs, LStringLen);
	}
}

////////////////////////////////////////
//                RNG                 //
////////////////////////////////////////

u32 *eit___GetPCGState__(void)
{
	static uint32_t eit___PCGState = 0;
	return &eit___PCGState;
}

// Implementation of the PCG algorithm (https://www.pcg-random.org)
// It's the caller's responsibilites to have called SeedRandPCG before use
u32 eit_RandPCG(void)
{
	u32 State = *eit___GetPCGState__();
	*eit___GetPCGState__() = State * 747796405u + 2891336453u;
	u32 Word = ((State >> ((State >> 28u) + 4u)) ^ State) * 277803737u;
	return (Word >> 22u) ^ Word;
}

void eit_SeedRandPCG(uint32_t Seed)
{
	*eit___GetPCGState__() = Seed;
}

/////////////////////////////////////////
//              FILE IO                //
/////////////////////////////////////////

void eit_MemUnmap(sdb_mmap *Map)
{
	if (Map->Data != NULL && Map->Data != MAP_FAILED) {
		munmap(Map->Data, Map->Size);
	}

	if (Map->Fd >= 0) {
		close(Map->Fd);
	}
}

sdb_errno eit___MemMap__(sdb_mmap *Map, void *MAddr, size_t MSize, int MProt,
			 int MFlags, int MFd, off_t MOffset,
			 sdb_string FileName, int OFlags, mode_t OMode,
			 sdb__log_module__ *Module)
{
	Map->Size = MSize;
	Map->Fd = -1;

	if (MFd < -1) {
		if (FileName) {
			Map->Filename = FileName;
			if (!Map->Filename) {
				eit___WriteLog__(Module, "ERR",
						 "Failed to copy filename: %s",
						 strerror(errno));
				goto error;
			}
		}

		Map->Fd = open(FileName, OFlags & (~O_TRUNC), OMode);
		if (Map->Fd == -1) {
			eit___WriteLog__(Module, "ERR",
					 "Error opening file %s: %s", FileName,
					 strerror(errno));
			goto error;
		}
	} else {
		Map->Fd = MFd;
	}

	if (OFlags & O_TRUNC || (MFlags & MAP_SHARED && MProt & PROT_WRITE)) {
		if (ftruncate(Map->Fd, MSize) == -1) {
			eit___WriteLog__(Module, "ERR",
					 "Error setting file size: %s",
					 strerror(errno));
			goto error;
		}

		if (lseek(Map->Fd, 0, SEEK_SET) == -1) {
			eit___WriteLog__(Module, "ERR",
					 "Error resetting file pointer: %s",
					 strerror(errno));
			goto error;
		}
	}

	if (MFlags & MAP_ANONYMOUS) {
		if (Map->Fd != -1) {
			close(Map->Fd);
			Map->Fd = -1;
		}
	}

	Map->Data = mmap(MAddr, MSize, MProt, MFlags, Map->Fd, MOffset);
	if (Map->Data == MAP_FAILED) {
		eit___WriteLog__(Module, "ERR", "Error mapping memory: %s",
				 strerror(errno));
		goto error;
	}

	if (MFlags & MAP_PRIVATE) {
		if (Map->Fd != -1) {
			close(Map->Fd);
			Map->Fd = -1;
		}
	}

	if (MFlags & MAP_LOCKED) {
		if (mlock(Map->Data, MSize) == -1) {
			eit___WriteLog__(Module, "WARN",
					 "Failed to lock pages in memory: %s",
					 strerror(errno));
		}
	}

	return 0;

error:
	eit_MemUnmap(Map);
	return -errno;
}

int eit_MemMapSync(sdb_mmap *Map)
{
	int Ret = msync(Map->Data, Map->Size, MS_SYNC);
	return Ret;
}

int eit_MemMapAdvise(sdb_mmap *Map, int Advice)
{
	int Ret = madvise(Map->Data, Map->Size, Advice);
	return Ret;
}

sdb_file_data *eit_LoadFileIntoMemory(const char *Filename, sdb_arena *Arena)
{
	FILE *File = fopen(Filename, "rb");
	if (!File) {
		printf("Failed to open file: %s\n", strerror(errno));
		return NULL;
	}

	if (fseek(File, 0L, SEEK_END) != 0) {
		fclose(File);
		return NULL;
	}

	u64 FileSize = (u64)ftell(File);
	rewind(File);

	sdb_file_data *FileData;
	u64 FileDataSize = sizeof(sdb_file_data) + FileSize + 1;
	if (NULL != Arena) {
		FileData = eit_ArenaPush(Arena, FileDataSize);
	} else {
		FileData = calloc(1, FileDataSize);
	}
	if (!FileData) {
		fclose(File);
		return NULL;
	}

	FileData->Size = FileSize;
	u64 BytesRead = fread(FileData->Data, 1, FileSize, File);
	if (BytesRead != FileSize) {
		fprintf(stderr, "Failed to read data from file %s\n",
			strerror(errno));
		fclose(File);
		if (Arena != NULL) {
			eit_ArenaPop(Arena, FileDataSize);
		} else {
			free(FileData);
		}
		return NULL;
	}

	fclose(File);
	return FileData;
}

bool eit_WriteBufferToFile(void *Buffer, u64 ElementSize, u64 ElementCount,
			   const char *Filename)
{
	FILE *fd = fopen(Filename, "wb");
	if (!fd) {
		fprintf(stderr, "Unable to open file %s!\n", Filename);
		return false;
	}

	bool WriteSuccessful = fwrite(Buffer, ElementSize, ElementCount, fd) ==
			       ElementCount;
	fclose(fd);
	return WriteSuccessful;
}

////////////////////////////////////////
//            "TOKENIZER"             //
////////////////////////////////////////

sdb_token eit_GetNextToken(char **Cursor)
{
	while ('\t' != **Cursor) {
		(*Cursor)++;
	}

	(*Cursor)++; // Skips to start of hex number

	sdb_token Token;
	Token.Start = *Cursor;
	Token.Len = 0;

	while ('\n' != **Cursor && '\r' != **Cursor) {
		(*Cursor)++;
		++Token.Len;
	}

	if ('\0' != **Cursor) {
		**Cursor = '\0';
	}

	return Token;
}
////////////////////////////////////////
//            MEM TRACE               //
////////////////////////////////////////

void *eit___MallocTrace__(size_t Size, int Line, const char *Func,
			  sdb__log_module__ *Module)
{
	void *Pointer = malloc(Size);
	eit___WriteLog__(Module, "DBG", "MALLOC (%s,%d): %p (%zd B)", Func,
			 Line, Pointer, Size);
	return Pointer;
}

void *eit___CallocTrace__(u64 ElementCount, u64 ElementSize, int Line,
			  const char *Func, sdb__log_module__ *Module)
{
	void *Pointer = calloc(ElementCount, ElementSize);
	eit___WriteLog__(Module, "DBG",
			 "CALLOC (%s,%d): %p (%lu * %luB = %zd B)", Func, Line,
			 Pointer, ElementCount, ElementSize,
			 (size_t)(ElementCount * ElementSize));
	return Pointer;
}

void *eit___ReallocTrace__(void *Pointer, size_t Size, int Line,
			   const char *Func, sdb__log_module__ *Module)
{
	void *Original = Pointer;
	void *Realloc = NULL;
	if (Pointer != NULL) {
		Realloc = realloc(Pointer, Size);
	}
	eit___WriteLog__(Module, "DBG", "REALLOC (%s,%d): %p -> %p (%zd B)",
			 Func, Line, Original, Realloc, Size);
	return Realloc;
}

void eit___FreeTrace__(void *Pointer, int Line, const char *Func,
		       sdb__log_module__ *Module)
{
	eit___WriteLog__(Module, "DBG", "FREE (%s,%d): %p", Func, Line,
			 Pointer);
	if (Pointer != NULL) {
		free(Pointer);
	}
}

#if (EIT_MEM_TRACE == 1) && (EIT_LOG_LEVEL >= EIT_LOG_LEVEL_DBG)
#define malloc(Size) \
	eit___MallocTrace__(Size, __LINE__, __func__, Sdb__LogInstance__)
#define calloc(Count, Size) \
	eit___CallocTrace__(Count, Size, __LINE__, __func__, Sdb__LogInstance__)
#define realloc(Pointer, Size)                                  \
	eit___ReallocTrace__(Pointer, Size, __LINE__, __func__, \
			     Sdb__LogInstance__)
#define free(Pointer) \
	eit___FreeTrace__(Pointer, __LINE__, __func__, Sdb__LogInstance__)
#endif

// NOLINTEND(misc-definitions-in-headers)

#endif // EIT_H_IMPLEMENTATION
