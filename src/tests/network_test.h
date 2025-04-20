#ifndef NETWORK_TEST_H
#define NETWORK_TEST_H

#include <src/allocators/u_arena.h>
#include "tests.h"

// 56 bytes
typedef struct {
	uint32_t source_ip;
	uint32_t dest_ip;
	uint16_t source_port;
	uint16_t dest_port;
	uint32_t sequence_number;
	uint32_t ack_number;
	uint8_t data[32];
	uint8_t data_sz;
	uint8_t padding[3]; // Explicit padding for alignment
} NetworkPacket;

// 72 bytes
typedef struct {
	char name[32];
	char email[32];
	uint32_t age;
	uint32_t id;
} PersonRecord;

// 64 bytes
typedef struct {
	char filename[32];
	uint64_t size;
	uint64_t created_time;
	uint64_t modified_time;
	uint32_t permissions;
	uint32_t owner_id;
} FileRecord;

// 96 bytes
typedef struct {
	char username[32];
	char email[48];
	uint64_t user_id;
	uint32_t login_count;
	uint32_t permissions;
} UserAccount;

typedef struct AccountList {
	UserAccount *account;
	struct AccountList *next;
} AccountList;

typedef struct File {
	uint8_t *data;
} File;

typedef struct Disk {
	size_t file_count;
	size_t max_file_count;
	FileRecord *records;
	File *files;
} Disk;

void network_test(UArena *a, alloc_fn_t alloc_fn, const char *alloc_fn_name,
		  free_fn_t free_fn, realloc_fn_t realloc_fn, uint iterations,
		  const char *log_filename, const char *file_mode);
#endif
