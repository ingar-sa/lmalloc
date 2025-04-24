#ifndef TESTS_H
#define TESTS_H

#include <src/lm.h>
#include <src/allocators/u_arena.h>
#include <src/metrics/timing.h>
#include <src/allocators/allocator_wrappers.h>
#include <src/cJSON/cJSON.h>

// NOTE: (isa): Start structs created by claude
// 77 bytes - not aligned to 8-byte boundary
typedef struct {
	char identifier[13];
	uint64_t timestamps[8];
} TimeSeriesData;

// 32 bytes - aligned
typedef struct {
	uint64_t timestamp;
	uint64_t sequence;
	uint64_t source_id;
	uint32_t type;
	uint32_t flags;
} EventHeader;

// 56 bytes - aligned
typedef struct {
	uint64_t keys[4];
	uint32_t counts[4];
	uint32_t checksum;
} KeyStore;

// 128 bytes
typedef struct {
	char filename[64];
	uint32_t width;
	uint32_t height;
	uint64_t file_size;
	uint64_t created_time;
	uint64_t modified_time;
	uint32_t color_space;
	uint32_t compression;
	uint8_t bit_depth;
	uint8_t channels;
	uint8_t has_alpha;
	uint8_t padding;
	float dpi_x;
	float dpi_y;
} ImageMetadata;

// 16 bytes - aligned
typedef struct {
	double x;
	double y;
} Point2D;

// 24 bytes - aligned
typedef struct {
	double x;
	double y;
	double z;
} Point3D;

// 28 bytes - not aligned to 8-byte boundary
typedef struct {
	double position[3];
	uint32_t flags;
} PositionData;

// 16 bytes
typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
	float opacity;
} RgbaColor;

// 40 bytes
typedef struct {
	Point2D center;
	float radius;
	float thickness;
	int32_t segments;
	uint32_t color;
	uint8_t filled;
	uint8_t padding[3]; // Explicit padding for alignment
} CircleShape;

// 40 bytes - aligned
typedef struct {
	char name[32];
	uint64_t id;
} NamedEntity;
// NOTE: (isa): End structs created by Claude

// NOTE: (isa): Start arrays created by Claude
static size_t small_sizes[] = { 8,  13, 16, 27, 32,  45,  56,  64,
				71, 80, 91, 96, 103, 112, 125, 128 };

static size_t medium_sizes[] = {
	256,  300,  378,  512,	768,  818,  1000, 1024,
	1359, 1700, 2000, 2048, 2222, 2918, 3875, 4096
};

static size_t large_sizes[] = {
	LmKibiByte(16) + 35, /* ~16 KB (unaligned) */
	LmKibiByte(32), /* 32 KB (aligned) */
	LmKibiByte(64) + 127, /* ~64 KB (unaligned) */
#if 0
	LmKibiByte(128), /* 128 KB (aligned) */
	LmKibiByte(256) + 511, /* ~256 KB (unaligned) */
	LmKibiByte(512), /* 512 KB (aligned) */
	LmKibiByte(1024) + 973, /* ~1 MB (unaligned) */
	LmKibiByte(1536), /* 1.5 MB (aligned) */
	LmKibiByte(2048) + 1523, /* ~2 MB (unaligned) */
	LmKibiByte(2560), /* 2.5 MB (aligned) */
	LmKibiByte(3072) + 2731, /* ~3 MB (unaligned) */
	LmKibiByte(4096), /* 4 MB (aligned) */
	LmKibiByte(5120) + 3567, /* ~5 MB (unaligned) */
	LmKibiByte(6144), /* 6 MB (aligned) */
	LmKibiByte(7168) + 4095, /* ~7 MB (unaligned) */
	LmKibiByte(8192) /* 8 MB (aligned) */
#endif
};

// NOTE: (isa): End arrays created by Claude

typedef struct {
	size_t *array;
	size_t len;
	const char *name;
} array_test;

typedef int (*test_fn_t)(void *ctx, bool running_in_debugger);

struct test_definition {
	test_fn_t test;
	const char *test_name;
};

int run_tests(cJSON *conf);

struct ua_params {
	size_t arena_sz;
	size_t alignment;
	bool contiguous;
	bool mallocd;
};

#endif
