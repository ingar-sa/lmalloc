#ifndef SDHS_ARENA_H
#define SDHS_ARENA_H

#include <src/lm.h>

#ifndef SDHS_TEST_ARENA
#define SDHS_TEST_ARENA 2
#endif

#if SDHS_TEST_ARENA == 1

#include "karena.h"
#include "allocator_wrappers.h"

typedef KArena SdhsArena;
typedef KAScratch SdhsArenaScratch;

// TODO: (isa): Move to JSON
#define SDHS_ARENA_TEST_IS_CONTIGUOUS true
#define SDHS_ARENA_TEST_IS_MALLOCD false
#define SDHS_ARENA_TEST_ALIGNMENT 16

#define SDHS_ALLOC_FN ka_alloc_timed

#define ArenaCreate(cap, contiguous, mallocd, alignment) ka_create((cap))
#define ArenaDestroy(kap) ka_destroy((*kap))
#define ArenaBootstrap(ka, new_existing, cap, alignment) \
	ka_bootstrap((ka), (cap))
#define ArenaAlloc(ka, size) SDHS_ALLOC_FN(NULL, (ka), (size))
#define ArenaFree(ka) ka_free((ka))
#define ArenaPop(ka, size) ka_pop((ka), (size))
#define ArenaPos(ka) ka_pos((ka))
#define ArenaSeek(ka, pos) ka_seek((ka), (pos))
#define ArenaCap(ka) ka_size((ka))
#define ArenaBase(ka) ka_base((ka))
#define ArenaReserve(ka, sz) ka_reserve((ka), (sz))
#define ArenaPushArray(a, type, count) KaPushArray(a, type, count)
#define ArenaPushArrayZero(a, type, count) KaPushArrayZero(a, type, count)
#define ArenaPushStruct(a, type) KaPushStruct(a, type)
#define ArenaPushStructZero(a, type) KaPushStructZero(a, type)
#define THREAD_ARENAS_REGISTER(thread_name, count) \
	KA_THREAD_ARENAS_REGISTER(thread_name, count)
#define THREAD_ARENAS_EXTERN(thread_name) KA_THREAD_ARENAS_EXTERN(thread_name)
#define ThreadArenasInit(thread_name) KaThreadArenasInit(thread_name)
#define ThreadArenasInitExtern(thread_name) \
	KaThreadArenasInitExtern(thread_name)
#define ThreadArenasAdd(arena) KaThreadArenasAdd(arena)
#define ScratchBegin(arena) ka_scratch_begin(arena)
#define ScratchGet(conflicts, conflict_count) \
	KaScratchGet(conflicts, conflict_count)
#define ScratchRelease(scratch) ka_scratch_release(scratch)

#endif

#if SDHS_TEST_ARENA == 2

#include "u_arena.h"
#include "allocator_wrappers.h"

typedef UArena SdhsArena;
typedef UAScratch SdhsArenaScratch;

// TODO: (isa): Move to JSON
#define SDHS_ARENA_TEST_IS_CONTIGUOUS true
#define SDHS_ARENA_TEST_IS_MALLOCD false
#define SDHS_ARENA_TEST_ALIGNMENT 16

#define SDHS_ALLOC_FN ua_alloc_timed

#define ArenaCreate(cap, contiguous, mallocd, alignment) \
	ua_create(cap, contiguous, mallocd, alignment)
#define ArenaDestroy(uap) ua_destroy(uap)
#define ArenaBootstrap(ua, new_existing, cap, alignment) \
	ua_bootstrap(ua, new_existing, cap, alignment)
#define ArenaAlloc(ua, size) SDHS_ALLOC_FN(ua, NULL, size)
#define ArenaFree(ua) ua_free(ua)
#define ArenaPop(ua, size) ua_pop(ua, size)
#define ArenaPos(ua) ua_pos(ua)
#define ArenaSeek(ua, pos) ua_seek(ua, pos)
#define ArenaCap(ua) (ua)->cap
#define ArenaBase(ua) (ua)->mem
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
#define ScratchRelease(scratch) ua_scratch_release(scratch)

#endif // SDHS_TEST_U_ARENA
#endif
