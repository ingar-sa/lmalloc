#ifndef SYSTEM_INFO_H
#define SYSTEM_INFO_H

#include <unistd.h>

struct cache_info {
	ssize_t l1d_cache_size; // L1 data cache size in bytes
	ssize_t l1d_cache_line; // L1 data cache line size in bytes
	ssize_t l1d_cache_assoc; // L1 data cache associativity

	ssize_t l1i_cache_size; // L1 instruction cache size in bytes
	ssize_t l1i_cache_line; // L1 instruction cache line size in bytes
	ssize_t l1i_cache_assoc; // L1 instruction cache associativity

	ssize_t l2_cache_size; // L2 cache size in bytes
	ssize_t l2_cache_line; // L2 cache line size in bytes
	ssize_t l2_cache_assoc; // L2 cache associativity

	ssize_t l3_cache_size; // L3 cache size in bytes
	ssize_t l3_cache_line; // L3 cache line size in bytes
	ssize_t l3_cache_assoc; // L3 cache associativity
};

size_t get_page_size(void);
size_t get_l1d_cacheln_sz(void);

struct cache_info get_cpu_cache_info(void);
void print_cache_info(struct cache_info info);

#endif
