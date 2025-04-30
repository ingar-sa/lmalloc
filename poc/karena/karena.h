#pragma once
#include <linux/ioctl.h>

#define LM_H_IMPLEMENTATION
// #include "lm.h"
#undef LM_H_IMPLEMENTATION

#define KARENA_MAGIC 'K'

#define KARENA_CREATE _IOWR(KARENA_MAGIC, 1, struct ka_data)
#define KARENA_ALLOC _IOWR(KARENA_MAGIC, 2, struct ka_data)
#define KARENA_SEEK _IOWR(KARENA_MAGIC, 4, struct ka_data)
#define KARENA_POP _IOWR(KARENA_MAGIC, 5, struct ka_data)
#define KARENA_POS _IOWR(KARENA_MAGIC, 6, struct ka_data)
#define KARENA_RESERVE _IOWR(KARENA_MAGIC, 7, struct ka_data)
#define KARENA_DESTROY _IOWR(KARENA_MAGIC, 8, struct ka_data)
#define KARENA_SIZE _IOWR(KARENA_MAGIC, 9, struct ka_data)
#define KARENA_BOOTSTRAP _IOWR(KARENA_MAGIC, 10, struct ka_data)

typedef unsigned long KArena;

struct ka_data {
	size_t size;
	unsigned long arena;
};

typedef struct {
	KArena *ka;
	size_t f5;
} KAScratch;

struct ka__thread_arenas__ {
	KArena **kas;
	int count;
	int max_count;
};

#define LM__CONCAT3__(x, y, z) x##y##z
#define LM_CONCAT3(x, y, z) LM__CONCAT3__(x, y, z)

#define KA_THREAD_ARENAS_REGISTER(thread_name, count_)                      \
	static __thread KArena *LM_CONCAT3(ka__thread_arenas_, thread_name, \
					   _buffer__)[count_]               \
		__attribute__((used));                                      \
	__thread struct ka__thread_arenas__ LM_CONCAT3(ka__thread_arenas_,  \
						       thread_name, __)     \
		__attribute__((used)) = { .kas = NULL,                      \
					  .count = 0,                       \
					  .max_count = count_ };            \
	static __thread struct ka__thread_arenas__                          \
		*ka__thread_arenas_instance__ __attribute__((used))

#define KA_THREAD_ARENAS_EXTERN(thread_name)                   \
	extern __thread struct ka__thread_arenas__ LM_CONCAT3( \
		ka__thread_arenas_, thread_name, __);          \
	static __thread struct ka__thread_arenas__             \
		*ka__thread_arenas_instance__ __attribute__((used))

#define KaThreadArenasInit(thread_name)                                 \
	ka__thread_arenas_init__(                                       \
		LM_CONCAT3(ka__thread_arenas_, thread_name, _buffer__), \
		&LM_CONCAT3(ka__thread_arenas_, thread_name, __),       \
		&ka__thread_arenas_instance__)

#define KaThreadArenasInitExtern(thread_name)                           \
	ka__thread_arenas_init_extern__(&LM_CONCAT3(ka__thread_arenas_, \
						    thread_name, __),   \
					&ka__thread_arenas_instance__)

#define KaThreadArenasAdd(arena) \
	ka__thread_arenas_add__(arena, ka__thread_arenas_instance__)

#define KaScratchGet(conflicts, conflict_count)      \
	ka__scratch_get__(conflicts, conflict_count, \
			  ka__thread_arenas_instance__)
