#ifndef SDHS_ARENA_H
#define SDHS_ARENA_H

#include <src/lm.h>

#ifndef SDHS_TEST_U_ARENA
#define SDHS_TEST_U_ARENA 1
#endif

#if SDHS_TEST_U_ARENA == 1

#include "u_arena.h"
#include "allocator_wrappers.h"

typedef UArena SdhsArena;
typedef UAScratch SdhsArenaScratch;

#define SDHS_ARENA_TEST_IS_CONTIGUOUS true
#define SDHS_ARENA_TEST_IS_MALLOCD false
#define SDHS_ARENA_TEST_ALIGNMENT 16

#define ArenaCreate(cap, contiguous, mallocd, alignment) \
	ua_create(cap, contiguous, mallocd, alignment)
#define ArenaDestroy(uap) ua_destroy(uap)
#define ArenaBootstrap(ua, new_existing, cap, alignment) \
	ua_bootstrap(ua, new_existing, cap, alignment)
#define ArenaAlloc(ua, size) ua_alloc_wrapper_timed(ua, size)
#define ArenaFree(ua) ua_free(ua)
#define ArenaPop(ua, size) ua_pop(ua, size)
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
