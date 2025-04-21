#ifndef SDHS_ARENA_H
#define SDHS_ARENA_H

// NOTE: (isa): This file can only use relative paths for its includes of project files
// due to sdhs being compiled separately and having conflicting include paths with the validation
#include "../lm.h"

#ifndef SDHS_TEST_U_ARENA
#define SDHS_TEST_U_ARENA 1
#endif

#if SDHS_TEST_U_ARENA == 1

#include "u_arena.h"
#include "allocator_wrappers.h"

typedef UArena SdhsArena;
typedef UArenaScratch SdhsArenaScratch;

#define SDHS_ARENA_TEST_IS_CONTIGUOUS true
#define SDHS_ARENA_TEST_IS_MALLOCD false
#define SDHS_ARENA_TEST_ALIGNMENT 16

#define ArenaInit(a, contiguous, mallocd, bootstrapped, alignment, page_sz,    \
		  cap, mem)                                                    \
	ua_init(a, contiguous, mallocd, bootstrapped, alignment, page_sz, cap, \
		mem)
#define ArenaCreate(cap, contiguous, mallocd, alignment) \
	ua_create(cap, contiguous, mallocd, alignment)
#define ArenaDestroy(uap) ua_destroy(uap)
#define ArenaBootstrap(ua, new_existing, cap, alignment) \
	ua_bootstrap(ua, new_existing, cap, alignment)
#define ArenaAlloc(ua, size) ua_alloc_wrapper_timed(ua, size)
#define ArenaZalloc(ua, size) ua_zalloc(ua, size)
#define ArenaFalloc(ua, size) ua_falloc(ua, size)
#define ArenaFzalloc(ua, size) ua_fzalloc(ua, size)
#define ArenaRelease(ua, pos, size) ua_release(ua, pos, size)
#define ArenaFree(ua) ua_free(ua)
#define ArenaPop(ua, size) ua_pop(ua, size)
#define ArenaSetAlignment(ua, alignment) ua_set_alignment(ua, alignment)
#define ArenaPos(ua) ua_pos(ua)
#define ArenaSeek(ua, pos) ua_seek(ua, pos)
#define ArenaReserve(ua, sz) ua_reserve(ua, sz)
#define ArenaPushArray(a, type, count) UaPushArray(a, type, count)
#define ArenaPushArrayZero(a, type, count) UaPushArrayZero(a, type, count)
#define ArenaPushStruct(a, type) UaPushStruct(a, type)
#define ArenaPushStructZero(a, type) UaPushStruct(a, type)
#define THREAD_ARENAS_REGISTER(thread_name, count) \
	UA_THREAD_ARENAS_REGISTER(thread_name, count)
#define THREAD_ARENAS_EXTERN(thread_name) UA_THREAD_ARENAS_EXTERN(thread_name)
#define ThreadArenasInit(thread_name) UaThreadArenasInit(thread_name)
#define ThreadArenasInitExtern(thread_name) \
	UaThreadArenasInitExtern(thread_name)
#define ThreadArenasAdd(arena) UaThreadArenasAdd(arena)
#define ScratchBegin(arena) ua_scratch_begin(arena)
#define ScratchGet(conflicts, conflict_count) \
	UaScratchGet(conflicts, conflict_count)
#define ScratchRelease(scratch) UaScratchRelease(scratch)

#endif // SDHS_TEST_U_ARENA

#endif
