/**
 * @file u_arena.h
 * @brief Arena allocator implementation
 */

#ifndef U_ARENA_H
#define U_ARENA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define U_ARENA_CONTIGUOUS true
#define U_ARENA_NON_CONTIGUOUS false
#define U_ARENA_MALLOCD true
#define U_ARENA_NOT_MALLOCD false

typedef struct u_arena {
	bool contiguous;
	bool mallocd;
	size_t alignment;
	size_t page_sz;
	size_t cap;
	size_t cur;
	uint8_t *mem;
} UArena;

void u_arena_init(UArena *a, bool contiguous, bool mallocd, size_t alignment,
		  size_t page_sz, size_t cap, uint8_t *mem);

UArena *u_arena_create(size_t cap, bool contiguous, bool mallocd,
		       size_t alignment);

void u_arena_destroy(UArena **ap);

void *u_arena_alloc(UArena *a, size_t size);

void *u_arena_zalloc(UArena *a, size_t size);

void *u_arena_falloc(UArena *a, size_t size);

void *u_arena_fzalloc(UArena *a, size_t size);

void u_arena_release(UArena *a, size_t pos, size_t size);

void u_arena_free(UArena *a);

void u_arena_pop(UArena *a, size_t size);

void u_arena_set_alignment(UArena *a, size_t alignment);

void *u_arena_pos(UArena *a);

void *u_arena_seek(UArena *a, size_t pos);

#define u_arena_array(a, type, count) u_arena_alloc(a, sizeof(type) * count)

#define u_arena_struct(a, type) u_arena_array(a, type, 1)

#endif /* u_arena_H */
