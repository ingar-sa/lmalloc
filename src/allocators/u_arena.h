/**
 * @file u_arena.h
 * @brief Arena allocator implementation
 */

#ifndef U_ARENA_H
#define U_ARENA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UA_CONTIGUOUS true
#define UA_NON_CONTIGUOUS false
#define UA_MALLOCD true
#define UA_MMAPD false

#define UA_CONTIGUOUS_BIT 0
#define UA_MALLOCD_BIT 1
#define UA_BOOTSTRAPPED_BIT 2

#define UaIsContiguous(flags) (!!((flags >> 0) & 1))
#define UaIsMallocd(flags) (!!((flags >> 1) & 1))
#define UaIsBootstrapped(flags) (!!((flags >> 2) & 1))

#define UaSetIsContiguous(flags) (flags |= 1)
#define UaSetIsMallocd(flags) (flags |= (1 << 1))
#define UaSetIsBootstrapped(flags) (flags |= (1 << 2))

typedef struct {
	uint_least64_t flags;
	size_t cap;
	size_t cur;
	uint8_t *mem;
} UArena;

typedef struct {
	UArena *ua;
	size_t f5;
} UAScratch;

struct ua__thread_arenas__ {
	UArena **uas;
	int count;
	int max_count;
};

#define UA_THREAD_ARENAS_REGISTER(thread_name, count_)                      \
	static __thread UArena *LM_CONCAT3(ua__thread_arenas_, thread_name, \
					   _buffer__)[count_]               \
		__attribute__((used));                                      \
	__thread struct ua__thread_arenas__ LM_CONCAT3(ua__thread_arenas_,  \
						       thread_name, __)     \
		__attribute__((used)) = { .uas = NULL,                      \
					  .count = 0,                       \
					  .max_count = count_ };            \
	static __thread struct ua__thread_arenas__                          \
		*ua__thread_arenas_instance__ __attribute__((used))

#define UA_THREAD_ARENAS_EXTERN(thread_name)                   \
	extern __thread struct ua__thread_arenas__ LM_CONCAT3( \
		ua__thread_arenas_, thread_name, __);          \
	static __thread struct ua__thread_arenas__             \
		*ua__thread_arenas_instance__ __attribute__((used))

// WARN: Must be used at runtime, not compile time. This is because you cannot take the address
// of a __thread varible in static initialization, since the address will differ per thread.
#define UaThreadArenasInit(thread_name)                                 \
	ua__thread_arenas_init__(                                       \
		LM_CONCAT3(ua__thread_arenas_, thread_name, _buffer__), \
		&LM_CONCAT3(ua__thread_arenas_, thread_name, __),       \
		&ua__thread_arenas_instance__)

#define UaThreadArenasInitExtern(thread_name)                           \
	ua__thread_arenas_init_extern__(&LM_CONCAT3(ua__thread_arenas_, \
						    thread_name, __),   \
					&ua__thread_arenas_instance__)

#define UaThreadArenasAdd(arena) \
	ua__thread_arenas_add__(arena, ua__thread_arenas_instance__)

#define UaScratchGet(conflicts, conflict_count)      \
	ua__scratch_get__(conflicts, conflict_count, \
			  ua__thread_arenas_instance__)

#define UaPushArray(a, type, count) ua_alloc(a, sizeof(type) * count)
#define UaPushArrayZero(a, type, count) ua_zalloc(a, sizeof(type) * count)

#define UaPushStruct(a, type) UaPushArray(a, type, 1)
#define UaPushStructZero(a, type) UaPushArrayZero(a, type, 1)

void ua_init(UArena *ua, bool contiguous, bool mallocd, bool bootstrapped,
	     size_t cap, uint8_t *mem);

UArena *ua_create(size_t cap, bool contiguous, bool mallocd);

void ua_destroy(UArena **uap);

UArena *ua_bootstrap(UArena *ua, UArena *new_existing, size_t cap);

void *ua_alloc(UArena *ua, size_t size);

void *ua_zalloc(UArena *ua, size_t size);

void *ua_falloc(UArena *ua, size_t size);

void *ua_fzalloc(UArena *ua, size_t size);

void ua_free(UArena *ua);

void ua_pop(UArena *ua, size_t size);

size_t ua_pos(UArena *ua);

void *ua_seek(UArena *ua, size_t pos);

size_t ua_reserve(UArena *ua, size_t sz);

typedef char *LmString;
LmString ua_info_string(UArena *ua, UArena *string_allocator);

void ua__thread_arenas_init__(UArena *ta_buf[], struct ua__thread_arenas__ *tas,
			      struct ua__thread_arenas__ **ta_instance);

void ua__thread_arenas_init_extern__(struct ua__thread_arenas__ *tas,
				     struct ua__thread_arenas__ **ta_instance);

int ua__thread_arenas_add__(UArena *ua,
			    struct ua__thread_arenas__ *ta_instance);

UAScratch ua_scratch_begin(UArena *ua);

void ua_scratch_release(UAScratch uas);

UAScratch ua__scratch_get__(UArena **conflicts, int conflict_count,
			    struct ua__thread_arenas__ *tas);

#endif /* u_arena_H */
