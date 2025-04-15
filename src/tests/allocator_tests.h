#ifndef ALLOCATOR_TESTS_H
#define ALLOCATOR_TESTS_H

#include <src/lm.h>

#include <src/munit/munit.h>
#include <src/cJSON/cJSON.h>

#include <stdbool.h>
#include <stdint.h>

// NOTE: (isa): Start structs created by claude

// 8 bytes
typedef struct {
	int x;
	int y;
} Point;

// 16 bytes
typedef struct {
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
	float opacity;
} RgbaColor;

// 24 bytes
typedef struct {
	Point p1;
	Point p2;
	Point p3;
} Triangle;

// 32 bytes
typedef struct {
	Point center;
	float radius;
	float thickness;
	int32_t segments;
	uint32_t color;
	uint8_t filled;
	uint8_t padding[3]; // Explicit padding for alignment
} CircleShape;

// 48 bytes
typedef struct {
	uint32_t source_ip;
	uint32_t dest_ip;
	uint16_t source_port;
	uint16_t dest_port;
	uint32_t sequence_number;
	uint32_t ack_number;
	uint8_t data[24];
	uint8_t flags;
	uint8_t padding[3]; // Explicit padding for alignment
} NetworkPacket;

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

// 8 bytes - aligned
typedef struct {
	int64_t value;
} SmallAligned;

// 12 bytes - not aligned to 8-byte boundary
typedef struct {
	int64_t id;
	uint32_t count;
} SmallUnaligned;

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

// 32 bytes - aligned
typedef struct {
	uint64_t timestamp;
	uint64_t sequence;
	uint64_t source_id;
	uint32_t type;
	uint32_t flags;
} EventHeader;

// 40 bytes - aligned
typedef struct {
	char name[32];
	uint64_t id;
} NamedEntity;

// 45 bytes - not aligned to 8-byte boundary
typedef struct {
	char tag[5];
	uint64_t values[5];
} TaggedValues;

// 56 bytes - aligned
typedef struct {
	uint64_t keys[4];
	uint32_t counts[4];
	uint32_t checksum;
} KeyStore;

// 64 bytes - aligned (cache line sized)
typedef struct {
	uint64_t data[8];
} CacheLineBlock;

// 72 bytes - aligned
typedef struct {
	double matrix[3][3];
} Matrix3x3;

// 77 bytes - not aligned to 8-byte boundary
typedef struct {
	char identifier[13];
	uint64_t timestamps[8];
} TimeSeriesData;

// 88 bytes - aligned
typedef struct {
	char name[32];
	uint32_t age;
	char address[48];
	uint32_t id;
} PersonRecord;

// 96 bytes - aligned
typedef struct {
	uint64_t nodes[12];
} GraphSegment;

// 103 bytes - not aligned to 8-byte boundary
typedef struct {
	double coordinates[12];
	uint8_t flags[7];
} GeometryObject;

// 120 bytes - aligned
typedef struct {
	char buffer[120];
} MessageBuffer;

// 128 bytes - aligned
typedef struct {
	uint64_t data[16];
} LargeBlock;

// NOTE: (isa): End structs created by Claude

// NOTE: (isa): Start arrays created by Claude
static size_t small_sizes[16] = { 8,  13, 16, 27, 32,  45,  56,	 64,
				  71, 80, 91, 96, 103, 112, 125, 128 };

static size_t medium_sizes[16] = { 256,	 300,  378,  512,  768,	 818,
				   1000, 1024, 1359, 1700, 2000, 2048,
				   2222, 2918, 3875, 4096 };

static size_t large_sizes[16] = {
	LmKibiByte(16) + 35, /* ~16 KB (unaligned) */
	LmKibiByte(32), /* 32 KB (aligned) */
	LmKibiByte(64) + 127, /* ~64 KB (unaligned) */
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
};

// NOTE: (isa): End arrays created by Claude

struct test_definition {
	MunitTestFunc test_fn;
	MunitTestSetup setup_fn;
	MunitTestTearDown teardown_fn;
	const char *test_name;
};

MunitSuite *create_munit_suite(cJSON *suite_conf);
MunitTest *get_suite_tests(cJSON *suite_tests_json);
struct test_definition *get_test_definition(cJSON *test_name_json);

struct u_arena_test_params {
	size_t arena_sz;
	size_t alignment;
	bool contiguous;
	bool mallocd;
	uint alloc_iterations;
	LmString log_filename;
	bool running_in_debugger;
};

struct malloc_test_params {
	uint alloc_iterations;
	LmString log_filename;
	bool running_in_debugger;
};

int u_arena_tests(struct u_arena_test_params *params);
int malloc_tests(struct malloc_test_params *param);
int malloc_tests_debug(struct malloc_test_params *params);
int u_arena_tests_debug(struct u_arena_test_params *params);

#endif
