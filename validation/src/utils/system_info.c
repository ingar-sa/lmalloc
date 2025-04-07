#include <src/lm.h>

LM_LOG_GLOBAL_DECLARE();
LM_LOG_REGISTER(system_info);

#include <unistd.h>
#include <errno.h>

#include "system_info.h"

// NOTE: Original written by claude
size_t get_page_size(void)
{
	ssize_t page_size = sysconf(_SC_PAGESIZE);

	if (page_size == -1) {
		if (errno == 0)
			LmLogWarning("Page size is indeterminate");
		else
			LmLogError("Failed to get page size (%s)",
				   strerror(errno));

		exit(EXIT_FAILURE);
	}

	return (size_t)page_size;
}

// NOTE: Original written by claude
struct cache_info get_cpu_cache_info(void)
{
	struct cache_info info = { 0 };

	// L1 Data Cache
	info.l1d_cache_size = sysconf(_SC_LEVEL1_DCACHE_SIZE);
	info.l1d_cache_line = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
	info.l1d_cache_assoc = sysconf(_SC_LEVEL1_DCACHE_ASSOC);

	// L1 Instruction Cache
	info.l1i_cache_size = sysconf(_SC_LEVEL1_ICACHE_SIZE);
	info.l1i_cache_line = sysconf(_SC_LEVEL1_ICACHE_LINESIZE);
	info.l1i_cache_assoc = sysconf(_SC_LEVEL1_ICACHE_ASSOC);

	// L2 Cache
	info.l2_cache_size = sysconf(_SC_LEVEL2_CACHE_SIZE);
	info.l2_cache_line = sysconf(_SC_LEVEL2_CACHE_LINESIZE);
	info.l2_cache_assoc = sysconf(_SC_LEVEL2_CACHE_ASSOC);

	// L3 Cache
	info.l3_cache_size = sysconf(_SC_LEVEL3_CACHE_SIZE);
	info.l3_cache_line = sysconf(_SC_LEVEL3_CACHE_LINESIZE);
	info.l3_cache_assoc = sysconf(_SC_LEVEL3_CACHE_ASSOC);

	return info;
}

size_t get_l1d_cacheln_sz(void)
{
	size_t l1d_cache_line = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
	return l1d_cache_line;
}

// NOTE: Original written by claude
void print_cache_info(struct cache_info info)
{
	printf("CPU Cache Information:\n");

	printf("\nL1 Data Cache:\n");
	printf("  Size: %ld bytes\n", info.l1d_cache_size);
	printf("  Line size: %ld bytes\n", info.l1d_cache_line);
	printf("  Associativity: %ld\n", info.l1d_cache_assoc);

	printf("\nL1 Instruction Cache:\n");
	printf("  Size: %ld bytes\n", info.l1i_cache_size);
	printf("  Line size: %ld bytes\n", info.l1i_cache_line);
	printf("  Associativity: %ld\n", info.l1i_cache_assoc);

	printf("\nL2 Cache:\n");
	printf("  Size: %ld bytes\n", info.l2_cache_size);
	printf("  Line size: %ld bytes\n", info.l2_cache_line);
	printf("  Associativity: %ld\n", info.l2_cache_assoc);

	printf("\nL3 Cache:\n");
	printf("  Size: %ld bytes\n", info.l3_cache_size);
	printf("  Line size: %ld bytes\n", info.l3_cache_line);
	printf("  Associativity: %ld\n", info.l3_cache_assoc);
}
